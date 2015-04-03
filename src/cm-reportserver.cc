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
#include "fnord-mdb/MDB.h"
#include "fnord-mdb/MDBUtil.h"
#include "common.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "JoinedQuery.h"
#include "reports/AnalyticsServlet.h"
#include "reports/CTRByPageServlet.h"
#include "reports/CTRStatsServlet.h"
#include "analytics/CTRByPositionRollup.h"
#include "analytics/AnalyticsQueryEngine.h"

using namespace fnord;

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
  fnord::thread::EventLoop ev;
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
    fnord::logInfo("cm.reportserver", "[VFS] Adding file: $0", file);
    return true;
  });

  /* analytics */
  cm::AnalyticsQueryEngine analytics(8, &vfs);
  cm::AnalyticsServlet analytics_servlet(&analytics);
  http_router.addRouteByPrefixMatch("/analytics", &analytics_servlet, &tpool);

  analytics.registerQueryFactory("ctr_by_position", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::CTRByPositionRollup(scan, segments);
  });

  ev.run();
  return 0;
}

