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
#include "fnord/base/io/filerepository.h"
#include "fnord/base/io/fileutil.h"
#include "fnord/base/application.h"
#include "fnord/base/random.h"
#include "fnord/base/thread/eventloop.h"
#include "fnord/base/thread/threadpool.h"
#include "fnord/comm/lbgroup.h"
#include "fnord/comm/rpc.h"
#include "fnord/cli/flagparser.h"
#include "fnord/comm/rpcchannel.h"
#include "fnord/json/json.h"
#include "fnord/json/jsonrpc.h"
#include "fnord/json/jsonrpchttpchannel.h"
#include "fnord/net/http/httprouter.h"
#include "fnord/net/http/httpserver.h"
#include "fnord/service/logstream/logstreamservice.h"
#include "fnord/service/logstream/feedfactory.h"
#include "fnord/stats/statshttpservlet.h"
#include "customernamespace.h"
#include "tracker/tracker.h"

using fnord::comm::LocalRPCChannel;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "public_http_port",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8080",
      "Start the public web interface on this port",
      "<port>");

  flags.defineFlag(
      "rpc_http_port",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8000",
      "Start the rpc http server on this port",
      "<port>");

  flags.parseArgv(argc, argv);

  fnord::thread::EventLoop event_loop;

  /* set up customers */
  auto dwn_ns = new cm::CustomerNamespace("dawanda");
  dwn_ns->addVHost("dwnapps.net");
  dwn_ns->loadTrackingJS("config/c_dwn/track.js");

  /* set up feedserver channel */
  fnord::comm::RoundRobinLBGroup feedserver_lbgroup;
  fnord::json::JSONRPCHTTPChannel feedserver_chan(
      &feedserver_lbgroup,
      &event_loop);

  feedserver_chan.httpConnectionPool()->stats()->exportStats(
      "/cm-frontend/http/outbound");

  feedserver_lbgroup.addServer("http://127.0.0.1:8001/rpc");

  /* set up tracker */
  fnord::logstream_service::LogStreamServiceFeedFactory feeds(&feedserver_chan);

  cm::Tracker tracker(&feeds);
  tracker.addCustomer(dwn_ns);

  /* set up public http server */
  fnord::http::HTTPRouter public_http_router;
  public_http_router.addRouteByPrefixMatch("/t", &tracker);
  fnord::http::HTTPServer public_http_server(&public_http_router, &event_loop);
  public_http_server.listen(flags.getInt("public_http_port"));
  public_http_server.stats()->exportStats("/cm/frontend/http/inbound");


  /* set up rpc http server */
  fnord::json::JSONRPC rpc;
  fnord::json::JSONRPCHTTPAdapter rpc_http(&rpc);

  fnord::http::HTTPRouter rpc_http_router;
  rpc_http_router.addRouteByPrefixMatch("/rpc", &rpc_http);
  fnord::http::HTTPServer rpc_http_server(&rpc_http_router, &event_loop);
  rpc_http_server.listen(flags.getInt("rpc_http_port"));

  fnord::stats::StatsHTTPServlet stats_servlet;
  rpc_http_router.addRouteByPrefixMatch("/stats", &stats_servlet);

  event_loop.run();
  return 0;
}

