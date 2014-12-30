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
#include "fnord/base/random.h"
#include "fnord/comm/rpc.h"
#include "fnord/cli/flagparser.h"
#include "fnord/comm/rpcchannel.h"
#include "fnord/io/filerepository.h"
#include "fnord/io/fileutil.h"
#include "fnord/json/json.h"
#include "fnord/json/jsonrpc.h"
#include "fnord/net/http/httprouter.h"
#include "fnord/net/http/httpserver.h"
#include "fnord/thread/eventloop.h"
#include "fnord/thread/threadpool.h"
#include "fnord/service/logstream/logstreamservice.h"
#include "fnord/service/logstream/feedfactory.h"
#include "customernamespace.h"
#include "tracker/tracker.h"
#include "tracker/logjoinservice.h"

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

  flags.defineFlag(
      "cmdata",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "clickmatcher app data dir",
      "<path>");

  flags.parseArgv(argc, argv);

  fnord::thread::ThreadPool thread_pool;
  fnord::thread::EventLoop event_loop;
  fnord::comm::RPCServiceMap service_map;

  fnord::json::JSONRPC rpc;
  fnord::json::JSONRPCHTTPAdapter rpc_http(&rpc);

  /* set up customers */
  auto dwn_ns = new cm::CustomerNamespace("dawanda");
  dwn_ns->addVHost("dwnapps.net");
  dwn_ns->loadTrackingJS("config/c_dwn/track.js");

  /* set up cmdata */
  auto cmdata_path = flags.getString("cmdata");
  if (!fnord::io::FileUtil::isDirectory(cmdata_path)) {
    RAISEF(kIOError, "no such directory: $0", cmdata_path);
  }

  /* set up logstream service */
  auto feeds_dir_path = fnord::io::FileUtil::joinPaths(cmdata_path, "feeds");
  fnord::io::FileUtil::mkdir_p(feeds_dir_path);

  fnord::logstream_service::LogStreamService logstream_service{
      fnord::io::FileRepository(feeds_dir_path)};

  rpc.registerService("LogStreamService", &logstream_service);

  /* set up tracker */
  auto logservice_chan =
      LocalRPCChannel::forService(&logstream_service, &thread_pool);
  fnord::logstream_service::LogStreamServiceFeedFactory feeds(
      logservice_chan.get());

  cm::Tracker tracker(&feeds);
  tracker.addCustomer(dwn_ns);

  /* set up public http server */
  fnord::http::HTTPRouter public_http_router;
  public_http_router.addRouteByPrefixMatch("/t", &tracker);
  fnord::http::HTTPServer public_http_server(&public_http_router, &event_loop);
  public_http_server.listen(flags.getInt("public_http_port"));

  /* set up rpc http server */
  fnord::http::HTTPRouter rpc_http_router;
  rpc_http_router.addRouteByPrefixMatch("/rpc", &rpc_http);
  fnord::http::HTTPServer rpc_http_server(&rpc_http_router, &event_loop);
  rpc_http_server.listen(flags.getInt("rpc_http_port"));

  event_loop.run();

  return 0;
}

