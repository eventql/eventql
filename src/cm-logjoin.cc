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
#include "fnord/io/filerepository.h"
#include "fnord/io/fileutil.h"
#include "fnord/application.h"
#include "fnord/logging.h"
#include "fnord/random.h"
#include "fnord/thread/eventloop.h"
#include "fnord/thread/threadpool.h"
#include "fnord/wallclock.h"
#include "fnord-rpc/ServerGroup.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord/cli/flagparser.h"
#include "fnord/json/json.h"
#include "fnord/json/jsonrpc.h"
#include "fnord/http/httprouter.h"
#include "fnord/http/httpserver.h"
#include "fnord-feeds/FeedService.h"
#include "fnord-feeds/RemoteFeedFactory.h"
#include "fnord-feeds/RemoteFeedReader.h"
#include "fnord/stats/statsdagent.h"
#include "fnord/http/httpconnectionpool.h"
#include "fnord/mdb/MDB.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include "CustomerNamespace.h"
#include "logjoin/LogJoin.h"
#include "logjoin/LogJoinTarget.h"
#include "logjoin/LogJoinUpload.h"
#include "DocStore.h"
#include "IndexChangeRequest.h"
#include "DocIndex.h"
#include "ItemRef.h"
#include "common.h"
#include "schemas.h"

using namespace cm;
using namespace fnord;

std::atomic<bool> cm_logjoin_shutdown;
fnord::thread::EventLoop ev;

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
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "upload target url",
      "<addr>");

  flags.defineFlag(
      "broker_addr",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "upload target url",
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
      "flush_interval",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "5",
      "flush_interval",
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
      "shard",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "shard",
      "<name>");

  flags.defineFlag(
      "loglevel",
      fnord::cli::FlagParser::T_STRING,
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
  loadDefaultSchemas(&schemas);

  /* start event loop */
  auto evloop_thread = std::thread([] {
    ev.run();
  });

  /* set up rpc client */
  HTTPRPCClient rpc_client(&ev);
  http::HTTPConnectionPool http(&ev);

  /* start stats reporting */
  fnord::stats::StatsdAgent statsd_agent(
      fnord::InetAddr::resolve(flags.getString("statsd_addr")),
      10 * fnord::kMicrosPerSecond);

  statsd_agent.start();

  /* get logjoin shard */
  cm::LogJoinShardMap shard_map;
  auto shard = shard_map.getShard(flags.getString("shard"));

  /* set up logjoin */
  auto dry_run = !flags.isSet("no_dryrun");
  size_t batch_size = flags.getInt("batch_size");
  size_t buffer_size = flags.getInt("buffer_size");
  size_t flush_interval = flags.getInt("flush_interval");

  fnord::logInfo(
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

  fnord::logInfo(
      "cm.logjoin",
      "Using session schema:\n$0",
      schemas.getSchema("cm.JoinedSession")->toString());

  /* open session db */
  auto sessdb = mdb::MDB::open(
      flags.getString("datadir"),
      false,
      1000000 * flags.getInt("db_size"),
      shard.shard_name + ".db",
      shard.shard_name + ".db.lck",
      false);

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
  feed_reader.setTimeBackfill([] (const feeds::FeedEntry& entry) -> DateTime {
    const auto& log_line = entry.data;

    auto c_end = log_line.find("|");
    if (c_end == std::string::npos) return 0;
    auto t_end = log_line.find("|", c_end + 1);
    if (t_end == std::string::npos) return 0;

    auto timestr = log_line.substr(c_end + 1, t_end - c_end - 1);
    return std::stoul(timestr) * fnord::kMicrosPerSecond;
  });

  /* open index */
  RefPtr<DocIndex> index(
      new DocIndex(
          flags.getString("index"),
          "documents-dawanda",
          true));

  /* set up logjoin target */
  fnord::fts::Analyzer analyzer(flags.getString("conf"));
  cm::LogJoinTarget logjoin_target(&schemas, dry_run);

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
      sessdb,
      flags.getString("tsdb_addr"),
      flags.getString("broker_addr"),
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

  DateTime last_flush;
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
        fnord::logError("cm.logjoin", e, "invalid log line");
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

    fnord::logInfo(
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
        fnord::logError("cm.logjoin", e, "upload failed");
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
  fnord::logInfo("cm.logjoin", "LogJoin exiting...");
  return 0;
}

