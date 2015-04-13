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
#include "fnord-base/VFS.h"
#include "fnord-rpc/ServerGroup.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-json/json.h"
#include "fnord-json/jsonrpc.h"
#include "fnord-http/httprouter.h"
#include "fnord-http/httpserver.h"
#include "fnord-http/VFSFileServlet.h"
#include "fnord-feeds/FeedService.h"
#include "fnord-feeds/RemoteFeedFactory.h"
#include "fnord-feeds/RemoteFeedReader.h"
#include "fnord-base/stats/statsdagent.h"
#include "fnord-sstable/SSTableServlet.h"
#include "fnord-eventdb/EventDBServlet.h"
#include "fnord-eventdb/TableRepository.h"
#include "fnord-eventdb/TableJanitor.h"
#include "fnord-mdb/MDB.h"
#include "fnord-mdb/MDBUtil.h"
#include "common.h"
#include "schemas.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "JoinedQuery.h"
#include "analytics/AnalyticsServlet.h"
#include "analytics/CTRByPageServlet.h"
#include "analytics/CTRStatsServlet.h"
#include "analytics/CTRByPositionQuery.h"
#include "analytics/CTRByPageQuery.h"
#include "analytics/TopSearchQueriesQuery.h"
#include "analytics/DiscoveryKPIQuery.h"
#include "analytics/DiscoveryCategoryStatsQuery.h"
#include "analytics/AnalyticsQueryEngine.h"
#include "analytics/AnalyticsQueryEngine.h"

using namespace fnord;

std::atomic<bool> shutdown_sig;
fnord::thread::EventLoop ev;

void quit(int n) {
  shutdown_sig = true;
  fnord::logInfo("cm.chunkserver", "Shutting down...");
  // FIXPAUL: wait for http server stop...
  ev.shutdown();
}

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  /* shutdown hook */
  shutdown_sig = false;
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = quit;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "http_port",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8000",
      "Start the public http server on this port",
      "<port>");

  flags.defineFlag(
      "replica",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "replica id",
      "<id>");

  flags.defineFlag(
      "artifacts",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "artifacts path",
      "<path>");

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

  WhitelistVFS vfs;

  /* start http server */
  fnord::thread::ThreadPool tpool;
  fnord::http::HTTPRouter http_router;
  fnord::http::HTTPServer http_server(&http_router, &ev);
  http_server.listen(flags.getInt("http_port"));

  /* sstable servlet */
  sstable::SSTableServlet sstable_servlet("/sstable", &vfs);
  http_router.addRouteByPrefixMatch("/sstable", &sstable_servlet);

  /* file servlet */
  http::VFSFileServlet file_servlet("/file", &vfs);
  http_router.addRouteByPrefixMatch("/file", &file_servlet);

  /* add all files to whitelist vfs */
  auto dir = flags.getString("artifacts");
  FileUtil::ls(dir, [&vfs, &dir] (const String& file) -> bool {
    vfs.registerFile(file, FileUtil::joinPaths(dir, file));
    fnord::logInfo("cm.chunkserver", "[VFS] Adding file: $0", file);
    return true;
  });

  /* eventdb */
  auto replica = flags.getString("replica");

  eventdb::TableRepository table_repo;
  table_repo.addTable(
      eventdb::Table::open(
          "dawanda_joined_sessions",
          replica,
          dir,
          joinedSessionsSchema()));

  eventdb::TableJanitor table_janitor(&table_repo);
  table_janitor.start();

  eventdb::EventDBServlet eventdb_servlet(&table_repo);

  /* analytics */
  cm::AnalyticsQueryEngine analytics(8, dir);
  cm::AnalyticsServlet analytics_servlet(&analytics);
  http_router.addRouteByPrefixMatch("/analytics", &analytics_servlet, &tpool);
  http_router.addRouteByPrefixMatch("/eventdb", &eventdb_servlet, &tpool);

  analytics.registerQueryFactory("ctr_by_position", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::CTRByPositionQuery(scan, segments);
  });

  analytics.registerQueryFactory("ctr_by_page", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::CTRByPageQuery(scan, segments);
  });

  analytics.registerQueryFactory("discovery_kpis", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::DiscoveryKPIQuery(
        scan,
        segments,
        query.start_time,
        query.end_time);
  });

  analytics.registerQueryFactory("discovery_category0_kpis", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::DiscoveryCategoryStatsQuery(
        scan,
        segments,
        "queries.category1",
        "queries.category1",
        params);
  });

  analytics.registerQueryFactory("discovery_category1_kpis", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::DiscoveryCategoryStatsQuery(
        scan,
        segments,
        "queries.category1",
        "queries.category2",
        params);
  });

  analytics.registerQueryFactory("discovery_category2_kpis", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::DiscoveryCategoryStatsQuery(
        scan,
        segments,
        "queries.category2",
        "queries.category3",
        params);
  });

  analytics.registerQueryFactory("discovery_category3_kpis", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::DiscoveryCategoryStatsQuery(
        scan,
        segments,
        "queries.category3",
        "queries.category3",
        params);
  });

  analytics.registerQueryFactory("top_search_queries", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::TopSearchQueriesQuery(scan, segments, params);
  });

  ev.run();

  table_janitor.stop();
  table_janitor.check();
  fnord::logInfo("cm.chunkserver", "Exiting...");

  return 0;
}

