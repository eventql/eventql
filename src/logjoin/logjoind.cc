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
#include "brokerd/FeedService.h"
#include "brokerd/RemoteFeedFactory.h"
#include "brokerd/RemoteFeedReader.h"
#include "stx/stats/statsdagent.h"
#include "stx/http/httpconnectionpool.h"
#include "stx/mdb/MDB.h"
#include "logjoin/LogJoin.h"
#include "logjoin/SessionProcessor.h"
#include "logjoin/LogJoinUpload.h"
#include "inventory/DocStore.h"
#include "inventory/IndexChangeRequest.h"
#include "inventory/DocIndex.h"
#include <inventory/ItemRef.h>
#include <fnord-fts/Analyzer.h>
#include "common/CustomerDirectory.h"
#include "common.h"

using namespace cm;
using namespace stx;

std::atomic<bool> cm_logjoin_shutdown;
stx::thread::EventLoop ev;

void quit(int n) {
  cm_logjoin_shutdown = true;
}

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  /* shutdown hook */
  cm_logjoin_shutdown = false;
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = quit;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "conf",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "./conf",
      "conf directory",
      "<path>");

  flags.defineFlag(
      "datadir",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "data dir",
      "<path>");

  flags.defineFlag(
      "index",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "index directory",
      "<path>");

  flags.defineFlag(
      "tsdb_addr",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "upload target url",
      "<addr>");

  flags.defineFlag(
      "broker_addr",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "upload target url",
      "<addr>");

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
      "2048",
      "batch_size",
      "<num>");

  flags.defineFlag(
      "buffer_size",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8192",
      "buffer_size",
      "<num>");

  flags.defineFlag(
      "flush_interval",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "5",
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

  /* load schemas */
  msg::MessageSchemaRepository schemas;

  schemas.registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedSession::descriptor()));

  schemas.registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedSearchQuery::descriptor()));

  schemas.registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedSearchQueryResultItem::descriptor()));

  schemas.registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedCartItem::descriptor()));

  schemas.registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedItemVisit::descriptor()));

  /* start event loop */
  auto evloop_thread = std::thread([] {
    ev.run();
  });

  /* set up rpc client */
  HTTPRPCClient rpc_client(&ev);
  http::HTTPConnectionPool http(&ev);

  /* start stats reporting */
  stx::stats::StatsdAgent statsd_agent(
      stx::InetAddr::resolve(flags.getString("statsd_addr")),
      10 * stx::kMicrosPerSecond);

  statsd_agent.start();

  /* get logjoin shard */
  cm::LogJoinShardMap shard_map;
  auto shard = shard_map.getShard(flags.getString("shard"));

  /* set up logjoin */
  auto dry_run = !flags.isSet("no_dryrun");
  size_t batch_size = flags.getInt("batch_size");
  size_t buffer_size = flags.getInt("buffer_size");
  size_t flush_interval = flags.getInt("flush_interval");

  stx::logInfo(
      "cm.logjoin",
      "Starting cm-logjoin with:\n    dry_run=$0\n    batch_size=$1\n" \
      "    buffer_size=$2\n    flush_interval=$9\n"
      "    max_dbsize=$4MB\n" \
      "    shard=$5\n    shard_range=[$6, $7)\n    shard_modulo=$8",
      dry_run,
      batch_size,
      buffer_size,
      0,
      flags.getInt("db_size"),
      shard.shard_name,
      shard.begin,
      shard.end,
      cm::LogJoinShard::modulo,
      flush_interval);

  stx::logInfo(
      "cm.logjoin",
      "Using session schema:\n$0",
      schemas.getSchema("cm.JoinedSession")->toString());

  /* open customer directory */
  CustomerDirectory customer_dir;

  {
    CustomerConfig dwn;
    dwn.set_customer("dawanda");
    auto hook = dwn.mutable_logjoin_config()->add_webhooks();
    hook->set_target_url("http://localhost:8080/mywebhook");
    customer_dir.updateCustomerConfig(dwn);
  }

  /* open session db */
  mdb::MDBOptions mdb_opts;
  mdb_opts.maxsize = 1000000 * flags.getInt("db_size"),
  mdb_opts.data_filename = shard.shard_name + ".db",
  mdb_opts.lock_filename = shard.shard_name + ".db.lck",
  mdb_opts.sync = false;

  auto sessdb = mdb::MDB::open(flags.getString("datadir"), mdb_opts);

  /* set up input feed reader */
  feeds::RemoteFeedReader feed_reader(&rpc_client);
  feed_reader.setMaxSpread(10 * kMicrosPerSecond);

  HashMap<String, URI> input_feeds;
  //input_feeds.emplace(
  //    "tracker_log.feedserver01.nue01.production.fnrd.net",
  //    URI("http://s01.nue01.production.fnrd.net:7001/rpc"));
  input_feeds.emplace(
      "tracker_log.feedserver02.nue01.production.fnrd.net",
      URI("http://s02.nue01.production.fnrd.net:7001/rpc"));

  input_feeds.emplace(
      "tracker_log.feedserver03.production.fnrd.net",
      URI("http://nue03.prod.fnrd.net:7001/rpc"));

  /* setup time backfill */
  feed_reader.setTimeBackfill([] (const feeds::FeedEntry& entry) -> UnixTime {
    const auto& log_line = entry.data;

    auto c_end = log_line.find("|");
    if (c_end == std::string::npos) return 0;
    auto t_end = log_line.find("|", c_end + 1);
    if (t_end == std::string::npos) return 0;

    auto timestr = log_line.substr(c_end + 1, t_end - c_end - 1);
    return std::stoul(timestr) * stx::kMicrosPerSecond;
  });

  /* open index */
  RefPtr<DocIndex> index(
      new DocIndex(
          flags.getString("index"),
          "documents-dawanda",
          true));

  /* set up logjoin target */
  stx::fts::Analyzer analyzer(flags.getString("conf"));
  cm::SessionProcessor logjoin_target(&schemas, dry_run);

  auto normalize = [&analyzer] (Language lang, const String& query) -> String {
    return analyzer.normalize(lang, query);
  };

  logjoin_target.setNormalize(normalize);
  logjoin_target.setGetField(std::bind(
      &DocIndex::getField,
      index.get(),
      std::placeholders::_1,
      std::placeholders::_2));

  /* set up logjoin upload */
  cm::LogJoinUpload logjoin_upload(
      &customer_dir,
      sessdb,
      flags.getString("tsdb_addr"),
      flags.getString("broker_addr"),
      &http);

  logjoin_target.start();

  /* setup logjoin */
  cm::LogJoin logjoin(shard, dry_run, &logjoin_target);
  logjoin.exportStats("/cm-logjoin/global");
  logjoin.exportStats(StringUtil::format("/cm-logjoin/$0", shard.shard_name));

  stx::stats::Counter<uint64_t> stat_stream_time_low;
  stx::stats::Counter<uint64_t> stat_stream_time_high;
  stx::stats::Counter<uint64_t> stat_active_sessions;
  stx::stats::Counter<uint64_t> stat_dbsize;

  exportStat(
      StringUtil::format("/cm-logjoin/$0/stream_time_low", shard.shard_name),
      &stat_stream_time_low,
      stx::stats::ExportMode::EXPORT_VALUE);

  exportStat(
      StringUtil::format("/cm-logjoin/$0/stream_time_high", shard.shard_name),
      &stat_stream_time_high,
      stx::stats::ExportMode::EXPORT_VALUE);

  exportStat(
      StringUtil::format("/cm-logjoin/$0/active_sessions", shard.shard_name),
      &stat_active_sessions,
      stx::stats::ExportMode::EXPORT_VALUE);

  exportStat(
      StringUtil::format("/cm-logjoin/$0/dbsize", shard.shard_name),
      &stat_dbsize,
      stx::stats::ExportMode::EXPORT_VALUE);


  /* upload pending q */
  if (!dry_run) {
    logjoin_upload.upload();
  }

  /* resume from last offset */
  auto txn = sessdb->startTransaction(true);
  try {
    logjoin.importTimeoutList(txn.get());

    for (const auto& input_feed : input_feeds) {
      uint64_t offset = 0;

      auto last_offset = txn->get(
          StringUtil::format("__logjoin_offset~$0", input_feed.first));

      if (!last_offset.isEmpty()) {
        offset = std::stoul(last_offset.get().toString());
      }

      stx::logInfo(
          "cm.logjoin",
          "Adding source feed:\n    feed=$0\n    url=$1\n    offset: $2",
          input_feed.first,
          input_feed.second.toString(),
          offset);

      feed_reader.addSourceFeed(
          input_feed.second,
          input_feed.first,
          offset,
          batch_size,
          buffer_size);
    }

    txn->abort();
  } catch (...) {
    txn->abort();
    throw;
  }

  stx::logInfo("cm.logjoin", "Resuming logjoin...");

  UnixTime last_flush;
  uint64_t rate_limit_micros = flush_interval * kMicrosPerSecond;

  for (;;) {
    auto begin = WallClock::unixMicros();

    feed_reader.fillBuffers();
    auto txn = sessdb->startTransaction();

    int i = 0;
    for (; i < batch_size; ++i) {
      auto entry = feed_reader.fetchNextEntry();

      if (entry.isEmpty()) {
        break;
      }

      try {
        logjoin.insertLogline(entry.get().data, txn.get());
      } catch (const std::exception& e) {
        stx::logError("cm.logjoin", e, "invalid log line");
      }
    }

    auto watermarks = feed_reader.watermarks();

    auto etime = WallClock::now().unixMicros() - last_flush.unixMicros();
    if (etime > rate_limit_micros) {
      last_flush = WallClock::now();
      logjoin.flush(txn.get(), watermarks.first);
    }

    auto stream_offsets = feed_reader.streamOffsets();
    String stream_offsets_str;

    for (const auto& soff : stream_offsets) {
      txn->update(
          StringUtil::format("__logjoin_offset~$0", soff.first),
          StringUtil::toString(soff.second));

      stream_offsets_str +=
          StringUtil::format("\n    offset[$0]=$1", soff.first, soff.second);
    }

    stx::logInfo(
        "cm.logjoin",
        "LogJoin comitting...\n    stream_time=<$0 ... $1>\n" \
        "    active_sessions=$2\n    flushed_sessions=$3$4",
        watermarks.first,
        watermarks.second,
        logjoin.numSessions(),
        logjoin_target.num_sessions,
        stream_offsets_str);

    if (dry_run) {
      txn->abort();
    } else {
      txn->commit();

      try {
        logjoin_upload.upload();
      } catch (const std::exception& e) {
        stx::logError("cm.logjoin", e, "upload failed");
      }
    }

    sessdb->removeStaleReaders();
    index->getDBHanndle()->removeStaleReaders();

    if (cm_logjoin_shutdown.load()) {
      break;
    }

    stat_stream_time_low.set(watermarks.first.unixMicros());
    stat_stream_time_high.set(watermarks.second.unixMicros());
    stat_active_sessions.set(logjoin.numSessions());
    //stat_dbsize.set(FileUtil::du_c(flags.getString("datadir"));

    auto rtime = WallClock::unixMicros() - begin;
    auto rlimit = kMicrosPerSecond;
    if (i < 2 && rtime < rlimit) {
      usleep(rlimit - rtime);
    }
  }

  ev.shutdown();
  evloop_thread.join();

  sessdb->sync();
  stx::logInfo("cm.logjoin", "LogJoin exiting...");

  logjoin_target.stop();
  return 0;
}

