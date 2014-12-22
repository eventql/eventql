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
#include "fnord/base/random.h"
#include "fnord/net/http/httprouter.h"
#include "fnord/net/http/httpserver.h"
#include "fnord/thread/eventloop.h"
#include "fnord/thread/threadpool.h"
#include "fnord/system/signalhandler.h"
#include "fnord/logging/logoutputstream.h"
#include "customernamespace.h"
#include "tracker/tracker.h"
#include "tracker/logjoinservice.h"

int main() {
  fnord::system::SignalHandler::ignoreSIGHUP();
  fnord::system::SignalHandler::ignoreSIGPIPE();

  fnord::CatchAndAbortExceptionHandler ehandler;
  ehandler.installGlobalHandlers();

  fnord::log::LogOutputStream logger(fnord::io::OutputStream::getStderr());
  fnord::log::Logger::get()->setMinimumLogLevel(fnord::log::kInfo);
  fnord::log::Logger::get()->listen(&logger);

  fnord::thread::ThreadPool thread_pool;
  fnord::thread::EventLoop event_loop;

  auto dwn_ns = new cm::CustomerNamespace();
  dwn_ns->addVHost("dwnapps.net");
  dwn_ns->loadTrackingJS("config/c_dwn/track.js");

  cm::LogJoinService logjoin_service(&thread_pool);
  cm::Tracker tracker(&logjoin_service);
  tracker.addCustomer(dwn_ns);

  fnord::http::HTTPRouter http_router;
  http_router.addRouteByPrefixMatch("/", &tracker);

  fnord::http::HTTPServer http_server(&http_router, &event_loop);
  http_server.listen(8080);

  event_loop.run();

  return 0;
}

