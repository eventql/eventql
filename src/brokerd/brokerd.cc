/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <unistd.h>
#include "stx/application.h"
#include "stx/io/filerepository.h"
#include "stx/io/fileutil.h"
#include "stx/thread/eventloop.h"
#include "stx/thread/threadpool.h"
#include "stx/random.h"
#include "stx/rpc/RPC.h"
#include "stx/cli/flagparser.h"
#include "stx/json/json.h"
#include "stx/json/jsonrpc.h"
#include "stx/http/httprouter.h"
#include "stx/http/httpserver.h"
#include "brokerd/FeedService.h"
#include "brokerd/RemoteFeedFactory.h"
#include "brokerd/BrokerServlet.h"
#include "stx/http/statshttpservlet.h"
#include "stx/stats/statsdagent.h"

using namespace stx;

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr();

  cli::FlagParser flags;

  flags.defineFlag(
      "http",
      cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8000",
      "Start the http server on this port",
      "<port>");

  flags.defineFlag(
      "loglevel",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.defineFlag(
      "datadir",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "data dir",
      "<path>");

  flags.defineFlag(
      "statsd",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "127.0.0.1:8192",
      "Submit statsd stats to this host:port",
      "<addr>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  thread::EventLoop event_loop;
  thread::ThreadPool tp(
      std::unique_ptr<ExceptionHandler>(
          new CatchAndLogExceptionHandler("cm.feedserver")));

  json::JSONRPC rpc;
  json::JSONRPCHTTPAdapter http(&rpc);

  /* set up logstream service */
  auto feeds_dir_path = flags.getString("datadir");
  FileUtil::mkdir_p(feeds_dir_path);

  feeds::FeedService service{
      feeds_dir_path,
      "/brokerd"};

  rpc.registerService(&service);

  /* set up rpc http server */
  http::HTTPRouter http_router;
  http_router.addRouteByPrefixMatch("/rpc", &http, &tp);
  http::HTTPServer http_server(&http_router, &event_loop);
  http_server.listen(flags.getInt("http"));
  http_server.stats()->exportStats("/brokerd/http");

  stats::StatsHTTPServlet stats_servlet;
  http_router.addRouteByPrefixMatch("/stats", &stats_servlet);

  feeds::BrokerServlet broker_servlet(&service);
  http_router.addRouteByPrefixMatch("/broker", &broker_servlet);

  stats::StatsdAgent statsd_agent(
      InetAddr::resolve(flags.getString("statsd")),
      10 * kMicrosPerSecond);

  statsd_agent.start();

  event_loop.run();
  return 0;
}

