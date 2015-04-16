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
#include "fnord-eventdb/TableReplication.h"
#include "fnord-eventdb/ArtifactReplication.h"
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
      "readonly",
      cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "readonly",
      "readonly");

  flags.defineFlag(
      "replica",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "replica id",
      "<id>");

  flags.defineFlag(
      "datadir",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "datadir path",
      "<path>");

  flags.defineFlag(
      "replicate_from",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "url",
      "<url>");


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

  /* start http server */
  fnord::thread::ThreadPool tpool;
  fnord::http::HTTPRouter http_router;
  fnord::http::HTTPServer http_server(&http_router, &ev);
  http_server.listen(flags.getInt("http_port"));

  /* eventdb */
  auto dir = flags.getString("datadir");
  auto readonly = flags.isSet("readonly");
  auto replica = flags.getString("replica");

  http::HTTPConnectionPool http(&ev);

  eventdb::ArtifactIndex artifacts(dir, replica, readonly);
  eventdb::TableRepository table_repo(&artifacts, dir, replica, readonly);
  table_repo.addTable("dawanda_joined_sessions", joinedSessionsSchema());

  eventdb::TableReplication table_replication(&http);
  for (const auto& rep : flags.getStrings("replicate_from")) {
    table_replication.replicateTableFrom(
        table_repo.findTableWriter("dawanda_joined_sessions"),
        URI(rep));
  }

  eventdb::ArtifactReplication artifact_replication(&artifacts, &http, 8);
  artifact_replication.addSource(
      URI("http://nue03.prod.fnrd.net:7005/chunks/"));

  eventdb::TableJanitor table_janitor(&table_repo);
  if (!readonly) {
    //table_janitor.start();
    //table_replication.start();
    artifact_replication.start();
  }

  eventdb::EventDBServlet eventdb_servlet(&table_repo);

  /* analytics */
  cm::AnalyticsQueryEngine analytics(8, dir, &table_repo);
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

  if (!readonly) {
    table_janitor.stop();
    table_janitor.check();
    table_replication.stop();
    artifact_replication.stop();
  }

  fnord::logInfo("cm.chunkserver", "Exiting...");

  exit(0);
}

