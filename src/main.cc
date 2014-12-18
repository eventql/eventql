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
#include "fnord/net/http/httpserver.h"
#include "fnord/thread/eventloop.h"
#include "fnord/thread/threadpool.h"
#include "fnord/system/signalhandler.h"
#include "fnord/logging/logoutputstream.h"
#include "customernamespace.h"
#include "tracker/tracker.h"

int main() {
  fnord::system::SignalHandler::ignoreSIGHUP();
  fnord::system::SignalHandler::ignoreSIGPIPE();

  fnord::CatchAndAbortExceptionHandler ehandler;
  ehandler.installGlobalHandlers();

  fnord::log::LogOutputStream logger(fnord::io::OutputStream::getStderr());
  fnord::log::Logger::get()->setMinimumLogLevel(fnord::log::kDebug);
  fnord::log::Logger::get()->listen(&logger);

  auto dwn_ns = new cm::CustomerNamespace();
  dwn_ns->addVHost("dwnapps.net");
  dwn_ns->loadTrackingJS("config/c_dwn/track.js");

  auto tracker = std::unique_ptr<cm::Tracker>(new cm::Tracker());
  tracker->addCustomer(dwn_ns);

  fnord::thread::ThreadPool thread_pool;
  fnord::thread::EventLoop event_loop;
  fnord::http::HTTPServer http_server(&thread_pool, &thread_pool);
  http_server.addHandler(std::move(tracker));
  http_server.listen(8080);

  for (;;) {
    usleep(10000);
  }

  return 0;
}

