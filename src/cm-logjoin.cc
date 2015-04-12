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
#include "fnord-base/io/filerepository.h"
#include "fnord-base/io/fileutil.h"
#include "fnord-base/application.h"
#include "fnord-base/logging.h"
#include "fnord-base/random.h"
#include "fnord-base/thread/eventloop.h"
#include "fnord-base/thread/threadpool.h"
#include "fnord-base/wallclock.h"
#include "fnord-rpc/ServerGroup.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-json/json.h"
#include "fnord-json/jsonrpc.h"
#include "fnord-http/httprouter.h"
#include "fnord-http/httpserver.h"
#include "fnord-feeds/FeedService.h"
#include "fnord-feeds/RemoteFeedFactory.h"
#include "fnord-feeds/RemoteFeedReader.h"
#include "fnord-base/stats/statsdagent.h"
#include "fnord-http/httpconnectionpool.h"
#include "fnord-mdb/MDB.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include "CustomerNamespace.h"
#include "logjoin/LogJoin.h"
#include "logjoin/LogJoinTarget.h"
#include "logjoin/LogJoinUpload.h"
#include "common.h"
#include "schemas.h"

using namespace cm;
using namespace fnord;

std::atomic<bool> cm_logjoin_shutdown;

void quit(int n) {
  cm_logjoin_shutdown = true;
}

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  /* shutdown hook */
  cm_logjoin_shutdown = false;
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = quit;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "conf",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "./conf",
      "conf directory",
      "<path>");

  flags.defineFlag(
      "cmdata",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "clickmatcher app data dir",
      "<path>");

  flags.defineFlag(
      "feedserver_addr",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      "http://localhost:8000",
      "feedserver addr",
      "<addr>");

  flags.defineFlag(
      "statsd_addr",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      "127.0.0.1:8192",
      "Statsd addr",
      "<addr>");

  flags.defineFlag(
      "batch_size",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "2048",
      "batch_size",
      "<num>");

  flags.defineFlag(
      "buffer_size",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8192",
      "buffer_size",
      "<num>");

  flags.defineFlag(
      "commit_size",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "1024",
      "commit_size",
      "<num>");

  flags.defineFlag(
      "commit_interval",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "5",
      "commit_interval",
      "<num>");

  flags.defineFlag(
      "db_size",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "128",
      "max sessiondb size",
      "<MB>");

   flags.defineFlag(
      "no_dryrun",
      fnord::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "no dryrun",
      "<bool>");

  flags.defineFlag(
      "loglevel",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.defineFlag(
      "shard",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "shard",
      "<name>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  /* schema... */
  auto joined_sessions_schema = joinedSessionsSchema();

  //fnord::iputs("$0", joined_sessions_schema.toString());

  /* set up cmdata */
  auto cmdata_path = flags.getString("cmdata");
  if (!FileUtil::isDirectory(cmdata_path)) {
    RAISEF(kIOError, "no such directory: $0", cmdata_path);
  }

  /* start event loop */
  fnord::thread::EventLoop ev;

  auto evloop_thread = std::thread([&ev] {
    ev.run();
  });

  /* set up rpc client */
  HTTPRPCClient rpc_client(&ev);
  http::HTTPConnectionPool http(&ev);

  /* start stats reporting */
  fnord::stats::StatsdAgent statsd_agent(
      fnord::net::InetAddr::resolve(flags.getString("statsd_addr")),
      10 * fnord::kMicrosPerSecond);

  statsd_agent.start();

  /* get logjoin shard */
  cm::LogJoinShardMap shard_map;
  auto shard = shard_map.getShard(flags.getString("shard"));

  /* set up logjoin */
  auto dry_run = !flags.isSet("no_dryrun");
  size_t batch_size = flags.getInt("batch_size");
  size_t buffer_size = flags.getInt("buffer_size");
  size_t commit_size = flags.getInt("commit_size");
  size_t commit_interval = flags.getInt("commit_interval");

  fnord::logInfo(
      "cm.logjoin",
      "Starting cm-logjoin with:\n    dry_run=$0\n    batch_size=$1\n" \
      "    buffer_size=$2\n    commit_size=$3\n    commit_interval=$9\n"
      "    max_dbsize=$4MB\n" \
      "    shard=$5\n    shard_range=[$6, $7)\n    shard_modulo=$8",
      dry_run,
      batch_size,
      buffer_size,
      commit_size,
      flags.getInt("db_size"),
      shard.shard_name,
      shard.begin,
      shard.end,
      cm::LogJoinShard::modulo,
      commit_interval);

  /* open session db */
  auto sessdb_path = FileUtil::joinPaths(
      cmdata_path,
      StringUtil::format("logjoin/$0/sessions_db", shard.shard_name));

  FileUtil::mkdir_p(sessdb_path);
  auto sessdb = mdb::MDB::open(sessdb_path);
  sessdb->setMaxSize(1000000 * flags.getInt("db_size"));

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

  /* setup time backfill */
  feed_reader.setTimeBackfill([] (const feeds::FeedEntry& entry) -> DateTime {
    const auto& log_line = entry.data;

    auto c_end = log_line.find("|");
    if (c_end == std::string::npos) return 0;
    auto t_end = log_line.find("|", c_end + 1);
    if (t_end == std::string::npos) return 0;

    auto timestr = log_line.substr(c_end + 1, t_end - c_end - 1);
    return std::stoul(timestr) * fnord::kMicrosPerSecond;
  });

  /* set up logjoin target */
  fnord::fts::Analyzer analyzer(flags.getString("conf"));
  cm::LogJoinTarget logjoin_target(joined_sessions_schema, &analyzer);

  /* set up logjoin upload */
  cm::LogJoinUpload logjoin_upload(
      sessdb,
      flags.getString("feedserver_addr"),
      &http);

  /* setup logjoin */
  cm::LogJoin logjoin(shard, dry_run, &logjoin_target);
  logjoin.exportStats("/cm-logjoin/global");
  logjoin.exportStats(StringUtil::format("/cm-logjoin/$0", shard.shard_name));

  fnord::stats::Counter<uint64_t> stat_stream_time_low;
  fnord::stats::Counter<uint64_t> stat_stream_time_high;
  fnord::stats::Counter<uint64_t> stat_active_sessions;
  fnord::stats::Counter<uint64_t> stat_dbsize;

  exportStat(
      StringUtil::format("/cm-logjoin/$0/stream_time_low", shard.shard_name),
      &stat_stream_time_low,
      fnord::stats::ExportMode::EXPORT_VALUE);

  exportStat(
      StringUtil::format("/cm-logjoin/$0/stream_time_high", shard.shard_name),
      &stat_stream_time_high,
      fnord::stats::ExportMode::EXPORT_VALUE);

  exportStat(
      StringUtil::format("/cm-logjoin/$0/active_sessions", shard.shard_name),
      &stat_active_sessions,
      fnord::stats::ExportMode::EXPORT_VALUE);

  exportStat(
      StringUtil::format("/cm-logjoin/$0/dbsize", shard.shard_name),
      &stat_dbsize,
      fnord::stats::ExportMode::EXPORT_VALUE);


  /* upload pending q */
  logjoin_upload.upload();

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

      fnord::logInfo(
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

  fnord::logInfo("cm.logjoin", "Resuming logjoin...");

  DateTime last_iter;
  uint64_t rate_limit_micros = commit_interval * kMicrosPerSecond;

  for (;;) {
    last_iter = WallClock::now();
    feed_reader.fillBuffers();
    auto txn = sessdb->startTransaction();

    int i = 0;
    for (; i < commit_size; ++i) {
      auto entry = feed_reader.fetchNextEntry();

      if (entry.isEmpty()) {
        break;
      }

      try {
        logjoin.insertLogline(entry.get().data, txn.get());
      } catch (const std::exception& e) {
        fnord::logError("cm.logjoin", e, "invalid log line");
      }
    }

    auto watermarks = feed_reader.watermarks();
    logjoin.flush(txn.get(), watermarks.first);

    auto stream_offsets = feed_reader.streamOffsets();
    String stream_offsets_str;

    for (const auto& soff : stream_offsets) {
      txn->update(
          StringUtil::format("__logjoin_offset~$0", soff.first),
          StringUtil::toString(soff.second));

      stream_offsets_str +=
          StringUtil::format("\n    offset[$0]=$1", soff.first, soff.second);
    }

    fnord::logInfo(
        "cm.logjoin",
        "LogJoin comitting...\n    stream_time=<$0 ... $1>\n" \
        "    active_sessions=$2$3",
        watermarks.first,
        watermarks.second,
        logjoin.numSessions(),
        stream_offsets_str);

    txn->commit();

    try {
      logjoin_upload.upload();
    } catch (const std::exception& e) {
      fnord::logError("cm.logjoin", e, "upload failed");
    }

    stat_stream_time_low.set(watermarks.first.unixMicros());
    stat_stream_time_high.set(watermarks.second.unixMicros());
    stat_active_sessions.set(logjoin.numSessions());
    stat_dbsize.set(FileUtil::du_c(sessdb_path));

    if (cm_logjoin_shutdown.load() == true) {
      break;
    }

    auto etime = WallClock::now().unixMicros() - last_iter.unixMicros();
    if (i < 1 && etime < rate_limit_micros) {
      usleep(rate_limit_micros - etime);
    }
  }

  ev.shutdown();
  //evloop_thread.join();

  fnord::logInfo("cm.logjoin", "LogJoin exiting...");
  exit(0); // FIXPAUL

  return 0;
}

