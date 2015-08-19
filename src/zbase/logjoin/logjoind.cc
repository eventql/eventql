/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "stx/io/filerepository.h"
#include "stx/io/fileutil.h"
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/random.h"
#include "stx/thread/eventloop.h"
#include "stx/thread/threadpool.h"
#include "stx/wallclock.h"
#include "stx/rpc/ServerGroup.h"
#include "stx/rpc/RPC.h"
#include "stx/rpc/RPCClient.h"
#include "stx/cli/flagparser.h"
#include "stx/json/json.h"
#include "stx/json/jsonrpc.h"
#include "stx/http/httprouter.h"
#include "stx/http/httpserver.h"
#include "stx/stats/statsdagent.h"
#include "stx/http/httpconnectionpool.h"
#include "stx/mdb/MDB.h"
#include "zbase/logjoin/LogJoin.h"
#include "zbase/logjoin/SessionProcessor.h"
#include "zbase/logjoin/stages/SessionJoin.h"
#include "zbase/logjoin/stages/BuildSessionAttributes.h"
#include "zbase/logjoin/stages/NormalizeQueryStrings.h"
#include "zbase/logjoin/stages/DebugPrintStage.h"
#include "zbase/logjoin/stages/DeliverWebhookStage.h"
#include "zbase/logjoin/stages/TSDBUploadStage.h"
#include "zbase/docdb/DocStore.h"
#include "zbase/docdb/IndexChangeRequest.h"
#include "zbase/docdb/DocIndex.h"
#include <zbase/docdb/ItemRef.h>
#include <zbase/util/Analyzer.h>
#include "zbase/ConfigDirectory.h"
#include "zbase/SessionSchema.h"
#include "common.h"

using namespace zbase;
using namespace stx;

stx::thread::EventLoop ev;
zbase::LogJoin* logjoin_instance;

void quit(int n) {
  logjoin_instance->shutdown();
}

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();
  stx::cli::FlagParser flags;

  //flags.defineFlag(
  //    "conf",
  //    cli::FlagParser::T_STRING,
  //    false,
  //    NULL,
  //    "./conf",
  //    "conf directory",
  //    "<path>");

  flags.defineFlag(
      "datadir",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "data dir",
      "<path>");

  flags.defineFlag(
      "tsdb_addr",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "upload target url",
      "<addr>");

  //flags.defineFlag(
  //    "broker_addr",
  //    stx::cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "upload target url",
  //    "<addr>");

  flags.defineFlag(
      "statsd_addr",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "127.0.0.1:8192",
      "Statsd addr",
      "<addr>");

  flags.defineFlag(
      "batch_size",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8192",
      "batch_size",
      "<num>");

  flags.defineFlag(
      "buffer_size",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "32000",
      "buffer_size",
      "<num>");

  flags.defineFlag(
      "flush_interval",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "30",
      "flush_interval",
      "<num>");

  flags.defineFlag(
      "db_size",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "128",
      "max sessiondb size",
      "<MB>");

   flags.defineFlag(
      "no_dryrun",
      stx::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "no dryrun",
      "<bool>");

  flags.defineFlag(
      "shard",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "shard",
      "<name>");

  flags.defineFlag(
      "master",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "url",
      "<addr>");

  flags.defineFlag(
      "loglevel",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  auto dry_run = !flags.isSet("no_dryrun");

  /* start event loop */
  auto evloop_thread = std::thread([] {
    ev.run();
  });

  http::HTTPConnectionPool http(&ev);

  /* start stats reporting */
  stx::stats::StatsdAgent statsd_agent(
      stx::InetAddr::resolve(flags.getString("statsd_addr")),
      10 * stx::kMicrosPerSecond);

  statsd_agent.start();

  /* get logjoin shard */
  zbase::LogJoinShardMap shard_map;
  auto shard = shard_map.getShard(flags.getString("shard"));

  /* open customer directory */
  auto cdb_dir = FileUtil::joinPaths(
      flags.getString("datadir"),
      shard.shard_name + "/cdb");

  if (!FileUtil::exists(cdb_dir)) {
    FileUtil::mkdir(cdb_dir);
  }

  ConfigDirectory customer_dir(
      cdb_dir,
      InetAddr::resolve(flags.getString("master")),
      ConfigTopic::CUSTOMERS);

  HashMap<String, URI> input_feeds;
  input_feeds.emplace(
      "tracker_log.feedserver02.nue01.production.fnrd.net",
      URI("http://s02.nue01.production.fnrd.net:7001/rpc"));

  input_feeds.emplace(
      "tracker_log.feedserver03.production.fnrd.net",
      URI("http://nue03.prod.fnrd.net:7001/rpc"));

  /* set up session processing pipeline */
  auto spool_path = FileUtil::joinPaths(
      flags.getString("datadir"),
      shard.shard_name + "/spool");

  FileUtil::mkdir_p(spool_path);
  zbase::SessionProcessor session_proc(&customer_dir, spool_path);

  /* pipeline stage: session join */
  session_proc.addPipelineStage(
      std::bind(&SessionJoin::process, std::placeholders::_1));

  /* pipeline stage: BuildSessionAttributes */
  session_proc.addPipelineStage(
      std::bind(&BuildSessionAttributes::process, std::placeholders::_1));

  /* pipeline stage: NormalizeQueryStrings */
  //stx::fts::Analyzer analyzer(flags.getString("conf"));
  //session_proc.addPipelineStage(
  //    std::bind(
  //        &NormalizeQueryStrings::process,
  //        NormalizeQueryStrings::NormalizeFn(
  //            std::bind(
  //                &stx::fts::Analyzer::normalize,
  //                &analyzer,
  //                std::placeholders::_1,
  //                std::placeholders::_2)),
  //        std::placeholders::_1));

  if (dry_run) {
    session_proc.addPipelineStage(
        std::bind(&DebugPrintStage::process, std::placeholders::_1));
  } else {
    /* pipeline stage: TSDBUpload */
    session_proc.addPipelineStage(
        std::bind(
            &TSDBUploadStage::process,
            std::placeholders::_1,
            flags.getString("tsdb_addr"),
            &http));

    /* pipeline stage: DeliverWebHook */
    session_proc.addPipelineStage(
        std::bind(&DeliverWebhookStage::process, std::placeholders::_1));
  }

  /* open session db */
  mdb::MDBOptions mdb_opts;
  mdb_opts.maxsize = 1000000 * flags.getInt("db_size");
  mdb_opts.data_filename = shard.shard_name + ".db";
  mdb_opts.lock_filename = shard.shard_name + ".db.lck";
  mdb_opts.duplicate_keys = false;
  auto sessdb = mdb::MDB::open(flags.getString("datadir"), mdb_opts);

  /* setup logjoin */
  zbase::LogJoin logjoin(shard, dry_run, sessdb, &session_proc, &ev);
  logjoin.exportStats("/logjoind/global");
  logjoin.exportStats(StringUtil::format("/logjoind/$0", shard.shard_name));

  /* shutdown hook */
  logjoin_instance = &logjoin;
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = quit;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  /* run logjoin */
  stx::logInfo(
      "logjoind",
      "Starting logjoind\n    dry_run=$0\n    shard=$1, [$2, $3) of [0, $4]",
      dry_run,
      shard.shard_name,
      shard.begin,
      shard.end,
      zbase::LogJoinShard::modulo);

  customer_dir.startWatcher();
  session_proc.start();
  logjoin.processClickstream(
      input_feeds,
      flags.getInt("batch_size"),
      flags.getInt("buffer_size"),
      flags.getInt("flush_interval") * kMicrosPerSecond);

  /* shutdown */
  stx::logInfo("logjoind", "LogJoin exiting...");
  session_proc.stop();
  ev.shutdown();
  evloop_thread.join();
  customer_dir.stopWatcher();
  sessdb->sync();
  exit(0);
}

