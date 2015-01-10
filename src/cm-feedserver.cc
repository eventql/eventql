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
#include "fnord/base/application.h"
#include "fnord/base/io/filerepository.h"
#include "fnord/base/io/fileutil.h"
#include "fnord/base/thread/eventloop.h"
#include "fnord/base/thread/threadpool.h"
#include "fnord/base/random.h"
#include "fnord/comm/rpc.h"
#include "fnord/cli/flagparser.h"
#include "fnord/comm/rpcchannel.h"
#include "fnord/json/json.h"
#include "fnord/json/jsonrpc.h"
#include "fnord/net/http/httprouter.h"
#include "fnord/net/http/httpserver.h"
#include "fnord/service/logstream/logstreamservice.h"
#include "fnord/service/logstream/feedfactory.h"
#include "fnord/stats/statshttpservlet.h"
#include "customernamespace.h"
#include "tracker/tracker.h"

using fnord::StringUtil;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "rpc_http_port",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8000",
      "Start the rpc http server on this port",
      "<port>");

  flags.defineFlag(
      "cmdata",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "clickmatcher app data dir",
      "<path>");

  flags.parseArgv(argc, argv);

  fnord::thread::EventLoop event_loop;

  fnord::json::JSONRPC rpc;
  fnord::json::JSONRPCHTTPAdapter rpc_http(&rpc);

  /* set up cmdata */
  auto cmdata_path = flags.getString("cmdata");
  if (!fnord::FileUtil::isDirectory(cmdata_path)) {
    RAISEF(kIOError, "no such directory: $0", cmdata_path);
  }

  /* set up logstream service */
  auto feeds_dir_path = fnord::FileUtil::joinPaths(cmdata_path, "feeds");
  fnord::FileUtil::mkdir_p(feeds_dir_path);

  fnord::logstream_service::LogStreamService logstream_service{
      fnord::FileRepository(feeds_dir_path),
      "/cm-feedserver/global/feeds"};

  rpc.registerService(&logstream_service);

  /* set up rpc http server */
  fnord::http::HTTPRouter rpc_http_router;
  rpc_http_router.addRouteByPrefixMatch("/rpc", &rpc_http);
  fnord::http::HTTPServer rpc_http_server(&rpc_http_router, &event_loop);
  rpc_http_server.listen(flags.getInt("rpc_http_port"));

  rpc_http_server.stats()->exportStats(
      "/cm-feedserver/global/http/inbound");
  rpc_http_server.stats()->exportStats(
      StringUtil::format("/cm-feedserver/$0/http/inbound", cm::cmHostname()));

  fnord::stats::StatsHTTPServlet stats_servlet;
  rpc_http_router.addRouteByPrefixMatch("/stats", &stats_servlet);

  event_loop.run();
  return 0;
}

