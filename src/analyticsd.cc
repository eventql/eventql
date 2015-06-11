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
#include "dproc/LocalScheduler.h"
#include "dproc/DispatchService.h"
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
#include "analytics/ReportFactory.h"
#include "analytics/FeedExportApp.h"
#include "analytics/ShopStatsServlet.h"
#include "analytics/AnalyticsApp.h"
#include "analytics/ShopStatsApp.h"

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
      "cachedir",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "cachedir path",
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

  /* load schemas */
  msg::MessageSchemaRepository schemas;
  loadDefaultSchemas(&schemas);

  fnord::logInfo(
      "analyticsd",
      "Using session schema:\n$0",
      schemas.getSchema("cm.JoinedSession")->toString());

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
  auto local_scheduler = mkRef(
      new dproc::LocalScheduler(
          flags.getString("cachedir"),
          12));

  local_scheduler->start();

  /* analytics core */
  auto analytics_app = mkRef(
      new AnalyticsApp(
          &tsdb,
          flags.getString("cachedir"),
          &schemas));

  auto shopstats_app = mkRef(new ShopStatsApp(&tsdb));

  dproc::DispatchService dproc;
  dproc.registerApp(analytics_app.get(), local_scheduler.get());
  dproc.registerApp(shopstats_app.get(), local_scheduler.get());

  cm::AnalyticsServlet analytics_servlet(analytics_app, &dproc);
  http_router.addRouteByPrefixMatch("/analytics", &analytics_servlet, &tpool);

  /* feed export */
  auto feed_export_app = mkRef(new FeedExportApp(&tsdb));
  dproc.registerApp(feed_export_app.get(), local_scheduler.get());

  {
    FeedConfig fc;
    fc.set_customer("dawanda");
    fc.set_feed("ECommerceSearchQueriesFeed");
    fc.set_partition_size(kMicrosPerHour * 4);
    fc.set_first_partition(1430438400000000); // 2015-05-01 00:00:00Z
    fc.set_num_shards(128);

    feed_export_app->configureFeed(fc);
  }

  {
    FeedConfig fc;
    fc.set_customer("dawanda");
    fc.set_feed("ECommerceRecoQueriesFeed");
    fc.set_partition_size(kMicrosPerHour * 4);
    fc.set_first_partition(1432785600000000);
    fc.set_num_shards(32);

    feed_export_app->configureFeed(fc);
  }

  {
    FeedConfig fc;
    fc.set_customer("dawanda");
    fc.set_feed("ECommercePreferenceSetsFeed");
    fc.set_partition_size(kMicrosPerHour * 4);
    fc.set_first_partition(1430438400000000); // 2015-05-01 00:00:00Z
    fc.set_num_shards(32);

    feed_export_app->configureFeed(fc);
  }

  /* stop stats */
  //cm::ShopStatsServlet shopstats_servlet(shopstats);
  //http_router.addRouteByPrefixMatch("/shopstats", &shopstats_servlet, &tpool);

  ev.run();
  local_scheduler->stop();

  fnord::logInfo("cm.analytics.backend", "Exiting...");
  exit(0);
}

