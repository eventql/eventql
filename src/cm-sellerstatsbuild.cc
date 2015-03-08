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
#include "fnord-mdb/MDB.h"
#include "CustomerNamespace.h"
#include "sellerstats/SellerStatsBuild.h"

using namespace fnord;

std::atomic<bool> shutdown;

void quit(int n) {
  shutdown = true;
}

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  /* shutdown hook */
  shutdown = false;
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = quit;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "cmdata",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "clickmatcher app data dir",
      "<path>");

  flags.defineFlag(
      "cmcustomer",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "clickmatcher customer",
      "<key>");

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
      "dbsize",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "128",
      "max statsdb size",
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

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

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

  /* start stats reporting */
  fnord::stats::StatsdAgent statsd_agent(
      fnord::net::InetAddr::resolve(flags.getString("statsd_addr")),
      10 * fnord::kMicrosPerSecond);

  statsd_agent.start();

  /* set up feature schema */
  cm::FeatureSchema feature_schema;
  feature_schema.registerFeature("shop_id", 1, 1);
  feature_schema.registerFeature("category1", 2, 1);
  feature_schema.registerFeature("category2", 3, 1);
  feature_schema.registerFeature("category3", 4, 1);

  /* args */
  auto dry_run = !flags.isSet("no_dryrun");
  auto cmcustomer = flags.getString("cmcustomer");
  size_t batch_size = flags.getInt("batch_size");
  size_t buffer_size = flags.getInt("buffer_size");
  size_t commit_size = flags.getInt("commit_size");
  size_t commit_interval = flags.getInt("commit_interval");

  /* seller activity log dir */
  auto sellerstatsactivitylog_path = FileUtil::joinPaths(
      cmdata_path,
      StringUtil::format("sellerstats/$0/activity_log", cmcustomer));

  FileUtil::mkdir_p(sellerstatsactivitylog_path);

  /* set up seller stats build */
  fnord::logInfo(
      "cm.sellerstats",
      "Starting cm.sellerstats with:\n    dry_run=$0\n    batch_size=$1\n"
      "    buffer_size=$2\n    commit_size=$3\n    commit_interval=$4",
      dry_run,
      batch_size,
      buffer_size,
      commit_size,
      commit_interval);

  cm::SellerStatsBuild sellerstats(&feature_schema);

  /* open sellerstats db */
  auto sellerstatsdb_path = FileUtil::joinPaths(
      cmdata_path,
      StringUtil::format("sellerstats/$0/db", cmcustomer));

  FileUtil::mkdir_p(sellerstatsdb_path);
  auto sellerstatsdb = mdb::MDB::open(sellerstatsdb_path);
  sellerstatsdb->setMaxSize(1000000 * flags.getInt("dbsize"));

  /* open featuredb db */
  auto featuredb_path = FileUtil::joinPaths(
      cmdata_path,
      StringUtil::format("index/$0/db", cmcustomer));

  auto featuredb = mdb::MDB::open(featuredb_path, true);
  auto featuredb_txn = featuredb->startTransaction(true);

  /* set up input feed reader */
  feeds::RemoteFeedReader item_visits_feed(&rpc_client);
  item_visits_feed.setMaxSpread(10 * kMicrosPerSecond);

  HashMap<String, URI> input_feeds;
  input_feeds.emplace(
      StringUtil::format(
          "$0.joined_item_visits.feedserver01.nue01.production.fnrd.net",
          cmcustomer),
      URI("http://s01.nue01.production.fnrd.net:7001/rpc"));
  input_feeds.emplace(
      StringUtil::format(
          "$0.joined_item_visits.feedserver02.nue01.production.fnrd.net",
          cmcustomer),
      URI("http://s02.nue01.production.fnrd.net:7001/rpc"));

  /* setup stats */
  fnord::stats::Counter<uint64_t> stat_stream_time_low;
  fnord::stats::Counter<uint64_t> stat_stream_time_high;
  fnord::stats::Counter<uint64_t> stat_active_sessions;
  fnord::stats::Counter<uint64_t> stat_dbsize;
/*
  exportStat(
      StringUtil::format("/cm-sellerstats/$0/stream_time_low", shard.shard_name),
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
*/

  /* resume from last offset */
  auto sellerstatsdb_txn = sellerstatsdb->startTransaction(true);
  try {
    for (const auto& input_feed : input_feeds) {
      uint64_t offset = 0;

      auto last_offset = sellerstatsdb_txn->get(
          StringUtil::format("__itemvisitfeed_offset~$0", input_feed.first));

      if (!last_offset.isEmpty()) {
        offset = std::stoul(last_offset.get().toString());
      }

      fnord::logInfo(
          "cm.sellerstats",
          "Adding source feed:\n    feed=$0\n    url=$1\n    offset: $2",
          input_feed.first,
          input_feed.second.toString(),
          offset);

      item_visits_feed.addSourceFeed(
          input_feed.second,
          input_feed.first,
          offset,
          batch_size,
          buffer_size);
    }

    sellerstatsdb_txn->abort();
  } catch (...) {
    sellerstatsdb_txn->abort();
    throw;
  }

  fnord::logInfo("cm.sellerstats", "Resuming SellerStatsBuild...");

  DateTime last_iter;
  uint64_t rate_limit_micros = commit_interval * kMicrosPerSecond;

  for (;;) {
    last_iter = WallClock::now();
    item_visits_feed.fillBuffers();
    auto sellerstatsdb_txn = sellerstatsdb->startTransaction();

    int i = 0;
    for (; i < commit_size; ++i) {
      auto entry = item_visits_feed.fetchNextEntry();

      if (entry.isEmpty()) {
        break;
      }

      try {
        sellerstats.insertJoinedItemVisit(
            fnord::json::fromJSON<cm::JoinedItemVisit>(entry.get().data),
            sellerstatsdb_txn.get(),
            featuredb_txn.get());
      } catch (const std::exception& e) {
        fnord::logError(
            "cm.sellerstats",
            e,
            "error while indexing joined item visit: $0",
            entry.get().data);
      }
    }

    auto watermarks = item_visits_feed.watermarks();
    //logjoin.flush(sellerstatsdb_txn.get(), watermarks.first);

    auto stream_offsets = item_visits_feed.streamOffsets();
    String stream_offsets_str;

    for (const auto& soff : stream_offsets) {
      sellerstatsdb_txn->update(
          StringUtil::format("__itemvisitfeed_offset~$0", soff.first),
          StringUtil::toString(soff.second));

      stream_offsets_str +=
          StringUtil::format("\n    offset[$0]=$1", soff.first, soff.second);
    }

    fnord::logInfo(
        "cm.sellerstats",
        "SellerStatsBuild comitting...\n    stream_time=<$0 ... $1>$2",
        watermarks.first,
        watermarks.second,
        stream_offsets_str);

    sellerstatsdb_txn->commit();

    stat_stream_time_low.set(watermarks.first.unixMicros());
    stat_stream_time_high.set(watermarks.second.unixMicros());
    stat_dbsize.set(FileUtil::du_c(sellerstatsdb_path));

    if (shutdown.load() == true) {
      break;
    }

    auto etime = WallClock::now().unixMicros() - last_iter.unixMicros();
    if (i < 1 && etime < rate_limit_micros) {
      usleep(rate_limit_micros - etime);
    }
  }

  ev.shutdown();
  //evloop_thread.join();

  featuredb_txn->abort();

  fnord::logInfo("cm.sellerstats", "SellerStatsBuild exiting...");
  exit(0); // FIXPAUL

  return 0;
}

