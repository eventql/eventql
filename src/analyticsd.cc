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
#include "fnord-base/thread/FixedSizeThreadPool.h"
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
#include "fnord-dproc/LocalScheduler.h"
#include "fnord-dproc/DispatchService.h"
#include "fnord-base/stats/statsdagent.h"
#include "fnord-sstable/SSTableServlet.h"
#include "fnord-logtable/LogTableServlet.h"
#include "fnord-logtable/TableRepository.h"
#include "fnord-logtable/TableJanitor.h"
#include "fnord-logtable/TableReplication.h"
#include "fnord-afx/ArtifactReplication.h"
#include "fnord-logtable/NumericBoundsSummary.h"
#include "fnord-mdb/MDB.h"
#include "fnord-mdb/MDBUtil.h"
#include "common.h"
#include "schemas.h"
#include "CustomerNamespace.h"
#include "analytics/AnalyticsServlet.h"
#include "analytics/FeedExportApp.h"
#include "analytics/CTRByPositionQuery.h"
#include "analytics/CTRByPageQuery.h"
#include "analytics/CTRByResultItemCategoryQuery.h"
#include "analytics/TopSearchQueriesQuery.h"
#include "analytics/DiscoveryDashboardQuery.h"
#include "analytics/DiscoveryCatalogStatsQuery.h"
#include "analytics/DiscoveryCategoryStatsQuery.h"
#include "analytics/DiscoverySearchStatsQuery.h"
#include "analytics/ECommerceKPIQuery.h"
#include "analytics/AnalyticsQueryEngine.h"
#include "analytics/AnalyticsQueryEngine.h"
#include "analytics/ShopStatsServlet.h"

using namespace fnord;

fnord::thread::EventLoop ev;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

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
      "datadir",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "datadir path",
      "<path>");

  flags.defineFlag(
      "shopstats_table",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "path",
      "<file>");

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

  /* thread pools */
  fnord::thread::ThreadPool tpool;
  fnord::thread::FixedSizeThreadPool wpool(8);
  wpool.start();

  /* http */
  fnord::http::HTTPRouter http_router;
  fnord::http::HTTPServer http_server(&http_router, &ev);
  http_server.listen(flags.getInt("http_port"));
  http::HTTPConnectionPool http(&ev);

  /* tsdb */
  tsdb::TSDBClient tsdb("http://nue03.prod.fnrd.net:7003/tsdb", &http);

  /* dproc */
  dproc::DispatchService dproc;
  auto local_scheduler = mkRef(new dproc::LocalScheduler());

  /* feed export */
  dproc.registerApp(new FeedExportApp(&tsdb), local_scheduler.get());

  /* stop stats */
  //auto shopstats = cm::ShopStatsTable::open(flags.getString("shopstats_table"));
  //cm::ShopStatsServlet shopstats_servlet(shopstats);
  //http_router.addRouteByPrefixMatch("/shopstats", &shopstats_servlet, &tpool);

  /* analytics core */
  auto dir = flags.getString("datadir");
  cm::AnalyticsQueryEngine analytics(32, dir, &tsdb);
  cm::AnalyticsServlet analytics_servlet(&analytics, &dproc);
  http_router.addRouteByPrefixMatch("/analytics", &analytics_servlet, &tpool);

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

  analytics.registerQueryFactory("ctr_by_result_item_category", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::CTRByResultItemCategoryQuery(scan, segments);
  });

  analytics.registerQueryFactory("discovery_dashboard", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::DiscoveryDashboardQuery(
        scan,
        segments,
        query.start_time,
        query.end_time);
  });

  analytics.registerQueryFactory("discovery_search_stats", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::DiscoverySearchStatsQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        params);
  });

  analytics.registerQueryFactory("discovery_catalog_stats", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::DiscoveryCatalogStatsQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        params);
  });

  analytics.registerQueryFactory("discovery_category0_stats", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::DiscoveryCategoryStatsQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        "search_queries.category1",
        "search_queries.category1",
        params);
  });

  analytics.registerQueryFactory("discovery_category1_stats", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::DiscoveryCategoryStatsQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        "search_queries.category1",
        "search_queries.category2",
        params);
  });

  analytics.registerQueryFactory("discovery_category2_stats", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::DiscoveryCategoryStatsQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        "search_queries.category2",
        "search_queries.category3",
        params);
  });

  analytics.registerQueryFactory("discovery_category3_stats", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::DiscoveryCategoryStatsQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        "search_queries.category3",
        "search_queries.category3",
        params);
  });

  analytics.registerQueryFactory("top_search_queries", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::TopSearchQueriesQuery(scan, segments, params);
  });

  analytics.registerQueryFactory("ecommerce_dashboard", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::ECommerceKPIQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        params);
  });

  ev.run();

  fnord::logInfo("cm.analytics.backend", "Exiting...");
  exit(0);
}

