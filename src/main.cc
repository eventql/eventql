/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stdlib.h>
#include "fnord/base/random.h"
#include "fnord/net/http/httpserver.h"
#include "fnord/thread/threadpool.h"
#include "fnord/system/signalhandler.h"
#include "customernamespace.h"
#include "tracker/tracker.h"

int main() {
  fnord::system::SignalHandler::ignoreSIGHUP();
  fnord::system::SignalHandler::ignoreSIGPIPE();
  fnord::CatchAndAbortExceptionHandler ehandler;
  ehandler.installGlobalHandlers();

  auto dwn_ns = new cm::CustomerNamespace();
  dwn_ns->addVHost("dwnapps.net");
  dwn_ns->addVHost("dwnapps.dev");
  dwn_ns->addVHost("dwnapps.dev:8080");
  dwn_ns->loadTrackingJS("config/c_dwn/track.js");

  auto tracker = std::unique_ptr<cm::Tracker>(new cm::Tracker());
  tracker->addCustomer(dwn_ns);

  fnord::thread::ThreadPool thread_pool;
  fnord::http::HTTPServer http_server(&thread_pool, &thread_pool);
  http_server.addHandler(std::move(tracker));
  http_server.listen(8080);

  return 0;
}

