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
#include "fnord-base/io/filerepository.h"
#include "fnord-base/io/fileutil.h"
#include "fnord-base/application.h"
#include "fnord-base/random.h"
#include "fnord-base/thread/eventloop.h"
#include "fnord-base/thread/threadpool.h"
#include "fnord-base/thread/queue.h"
#include "fnord-rpc/ServerGroup.h"
#include "fnord-rpc/RPC.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-json/json.h"
#include "fnord-json/jsonrpc.h"
#include "fnord-json/JSONRPCCodec.h"
#include "fnord-http/httprouter.h"
#include "fnord-http/httpserver.h"
#include "fnord-feeds/FeedService.h"
#include "fnord-feeds/RemoteFeedWriter.h"
#include "fnord-http/statshttpservlet.h"
#include "fnord-base/stats/statsdagent.h"
#include "fnord-rpc/RPCClient.h"
#include "CustomerNamespace.h"
#include "frontend/CMFrontend.h"
#include "frontend/IndexFeedUpload.h"
#include "SSETestServlet.h"

using namespace cm;
using namespace fnord;

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
      fnord::cli::FlagParser::T_STRING,
      true,
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

  thread::EventLoop event_loop;
  thread::ThreadPool tpool;
  http::HTTPConnectionPool http_client(&event_loop);
  HTTPRPCClient rpc_client(&event_loop);

  /* set up tracker log feed writer */
  feeds::RemoteFeedWriter tracker_log_feed(&rpc_client);
//  tracker_log_feed.addTargetFeed(
//      URI("http://s01.nue01.production.fnrd.net:7001/rpc"),
//      "tracker_log.feedserver01.nue01.production.fnrd.net",
//      16);

  tracker_log_feed.addTargetFeed(
      URI("http://s02.nue01.production.fnrd.net:7001/rpc"),
      "tracker_log.feedserver02.nue01.production.fnrd.net",
      16);

  tracker_log_feed.exportStats("/cm-frontend/global/tracker_log_writer");

  /* set up frontend */
  thread::Queue<IndexChangeRequest> indexfeed(8192);
  cm::CMFrontend frontend(&tracker_log_feed, &indexfeed);

  /* set up dawanda */
  auto dwn_ns = new cm::CustomerNamespace("dawanda");
  dwn_ns->addVHost("dwnapps.net");
  dwn_ns->loadTrackingJS("customers/dawanda/track.min.js");

  RefPtr<feeds::RemoteFeedWriter> dwn_index_request_feed(
      new feeds::RemoteFeedWriter(&rpc_client));

///  dwn_index_request_feed->addTargetFeed(
///      URI("http://s01.nue01.production.fnrd.net:7001/rpc"),
///      "index_requests.feedserver01.nue01.production.fnrd.net",
///      16);
///
  dwn_index_request_feed->addTargetFeed(
      URI("http://s02.nue01.production.fnrd.net:7001/rpc"),
      "index_requests.feedserver02.nue01.production.fnrd.net",
      16);

  dwn_index_request_feed->exportStats(
      "/cm-frontend/global/index_request_writer");

  frontend.addCustomer(dwn_ns, dwn_index_request_feed);

  /* set up public http server */
  http::HTTPRouter http_router;
  SSETestServlet sse_test;
  http_router.addRouteByPrefixMatch("/ssetest", &sse_test, &tpool);

  http_router.addRouteByPrefixMatch("/", &frontend);

  http::HTTPServer http_server(&http_router, &event_loop);
  http_server.listen(flags.getInt("http_port"));
  http_server.stats()->exportStats(
      "/cm-frontend/global/http/inbound");
  http_server.stats()->exportStats(
      StringUtil::format(
          "/cm-frontend/by-host/$0/http/inbound",
          cm::cmHostname()));


  /* start index feed upload */
  IndexFeedUpload indexfeed_upload(
      flags.getString("publish_to"),
      &indexfeed,
      &http_client);
  indexfeed_upload.start();


  /* stats reporting */
  stats::StatsdAgent statsd_agent(
      net::InetAddr::resolve(flags.getString("statsd_addr")),
      10 * kMicrosPerSecond);

  statsd_agent.start();
  event_loop.run();
  indexfeed_upload.stop();

  return 0;
}

