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
#include "eventql/util/application.h"
#include "eventql/util/io/filerepository.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/thread/eventloop.h"
#include "eventql/util/thread/threadpool.h"
#include "eventql/util/random.h"
#include "eventql/util/rpc/RPC.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/json/json.h"
#include "eventql/util/json/jsonrpc.h"
#include "eventql/util/http/httprouter.h"
#include "eventql/util/http/httpserver.h"
#include "brokerd/FeedService.h"
#include "brokerd/RemoteFeedFactory.h"
#include "eventql/util/http/statshttpservlet.h"
#include "eventql/util/stats/statsdagent.h"
#include "common.h"
#include "CustomerNamespace.h"

using namespace stx;

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "http_port",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8000",
      "Start the rpc http server on this port",
      "<port>");

  flags.defineFlag(
      "loglevel",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.defineFlag(
      "datadir",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "data dir",
      "<path>");

  flags.defineFlag(
      "statsd_addr",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "127.0.0.1:8192",
      "Statsd addr",
      "<addr>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  stx::thread::EventLoop event_loop;
  stx::thread::ThreadPool tp(
      std::unique_ptr<ExceptionHandler>(
          new CatchAndLogExceptionHandler("cm.feedserver")));

  stx::json::JSONRPC rpc;
  stx::json::JSONRPCHTTPAdapter http(&rpc);

  /* set up logstream service */
  auto feeds_dir_path = flags.getString("datadir");
  stx::FileUtil::mkdir_p(feeds_dir_path);

  stx::feeds::FeedService logstream_service{
      stx::FileRepository(feeds_dir_path),
      "/cm-feedserver/global/feeds"};

  rpc.registerService(&logstream_service);

  /* set up rpc http server */
  stx::http::HTTPRouter http_router;
  http_router.addRouteByPrefixMatch("/rpc", &http, &tp);
  stx::http::HTTPServer http_server(&http_router, &event_loop);
  http_server.listen(flags.getInt("http_port"));

  http_server.stats()->exportStats(
      "/cm-feedserver/global/http/inbound");
  http_server.stats()->exportStats(
      StringUtil::format("/cm-feedserver/$0/http/inbound", zbase::cmHostname()));

  stx::stats::StatsHTTPServlet stats_servlet;
  http_router.addRouteByPrefixMatch("/stats", &stats_servlet);

  stx::stats::StatsdAgent statsd_agent(
      stx::InetAddr::resolve(flags.getString("statsd_addr")),
      10 * stx::kMicrosPerSecond);

  statsd_agent.start();

  event_loop.run();
  return 0;
}

