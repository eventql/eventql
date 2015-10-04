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
#include "stx/io/filerepository.h"
#include "stx/io/fileutil.h"
#include "stx/application.h"
#include "stx/random.h"
#include "stx/thread/eventloop.h"
#include "stx/thread/threadpool.h"
#include "stx/thread/queue.h"
#include "stx/rpc/ServerGroup.h"
#include "stx/rpc/RPC.h"
#include "stx/cli/flagparser.h"
#include "stx/json/json.h"
#include "stx/json/jsonrpc.h"
#include "stx/json/JSONRPCCodec.h"
#include "stx/http/httprouter.h"
#include "stx/http/httpserver.h"
#include "brokerd/FeedService.h"
#include "brokerd/RemoteFeedWriter.h"
#include "stx/http/statshttpservlet.h"
#include "stx/stats/statsdagent.h"
#include "stx/rpc/RPCClient.h"
#include "zbase/tracker/TrackerServlet.h"

using namespace zbase;
using namespace stx;

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr();

  cli::FlagParser flags;

  flags.defineFlag(
      "http_port",
      cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8000",
      "Start the public http server on this port",
      "<port>");

  flags.defineFlag(
      "publish_to",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "upload target url",
      "<addr>");

  flags.defineFlag(
      "loglevel",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.defineFlag(
      "statsd_addr",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "127.0.0.1:8192",
      "Statsd addr",
      "<addr>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  /* load schemas */
  thread::EventLoop event_loop;
  thread::ThreadPool tpool;
  http::HTTPConnectionPool http_client(&event_loop);
  HTTPRPCClient rpc_client(&event_loop);

  /* set up tracker log feed writer */
  feeds::RemoteFeedWriter tracker_log_feed(&rpc_client);

  for (const auto& addr : flags.getStrings("publish_to")) {
    auto addr_parts = StringUtil::split(addr, "@");
    if (addr_parts.size() != 2) {
      RAISEF(
          kIllegalArgumentError,
          "illegal value for --publish_to: '$0', format is "
              "<feed_name>@<broker_addr>",
          addr);
    }

    tracker_log_feed.addTargetFeed(URI(addr_parts[1]), addr_parts[0], 16);
  }

  tracker_log_feed.exportStats("/cm-frontend/global/tracker_log_writer");

  /* set up frontend */
  zbase::TrackerServlet frontend(&tracker_log_feed);

  /* set up public http server */
  http::HTTPRouter http_router;
  http_router.addRouteByPrefixMatch("/", &frontend);

  http::HTTPServer http_server(&http_router, &event_loop);
  http_server.listen(flags.getInt("http_port"));
  http_server.stats()->exportStats("/ztracker/global/http/inbound");
  //http_server.stats()->exportStats(
  //    StringUtil::format(
  //        "/cm-frontend/by-host/$0/http/inbound",
  //        zbase::cmHostname()));


  /* stats reporting */
  stats::StatsdAgent statsd_agent(
      InetAddr::resolve(flags.getString("statsd_addr")),
      10 * kMicrosPerSecond);

  statsd_agent.start();
  event_loop.run();

  return 0;
}

