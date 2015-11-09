// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-base/executor/DirectExecutor.h>
#include <cortex-base/executor/NativeScheduler.h>
#include <cortex-base/net/Server.h>
#include <cortex-base/net/InetConnector.h>
#include <cortex-base/logging/LogAggregator.h>
#include <cortex-base/logging/LogTarget.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/WallClock.h>
#include <cortex-base/RuntimeError.h>

#include <cortex-http/HttpRequest.h>
#include <cortex-http/HttpResponse.h>
#include <cortex-http/HttpOutput.h>
#include <cortex-http/http1/ConnectionFactory.h>

enum class MaybeRaise {
  Never,
  Now,
};

class MaybeRaiseCategory : public std::error_category {
 public:
  static std::error_category& get() {
    static MaybeRaiseCategory ec;
    return ec;
  }
  const char* name() const noexcept override {
    return "maybe_raise";
  }
  std::string message(int ev) const override {
    switch (static_cast<MaybeRaise>(ev)) {
      case MaybeRaise::Now: return "Now";
      case MaybeRaise::Never: return "Never";
      default: return "Unknown MaybeRaise Error.";
    }
  }
};

using namespace cortex;
using namespace cortex::http;

int main() {
  auto errorHandler = [](const std::exception& e) {
    logAndPass(e);
  };

  WallClock* clock = WallClock::monotonic();
  NativeScheduler scheduler(errorHandler, clock);

  scheduler.setExceptionHandler(errorHandler);

  Server server;
  auto inet = server.addConnector<InetConnector>(
      "http", &scheduler, &scheduler, clock,
      TimeSpan::fromSeconds(30),
      TimeSpan::fromSeconds(30),
      TimeSpan::Zero,
      &logAndPass,
      IPAddress("0.0.0.0"), 3000, 128, true, false);
  auto http = inet->addConnectionFactory<http1::ConnectionFactory>(
      clock, 100, 512, 5, TimeSpan::fromMinutes(3));

  http->setHandler([](HttpRequest* request,
                      HttpResponse* response) {
    if (request->path() == "/raise") {
      printf("blah\n");
      RAISE_CATEGORY(MaybeRaise::Now, MaybeRaiseCategory::get());
    }

    response->setStatus(HttpStatus::Ok);
    response->output()->write("Call me maybe /raise ;-)\n",
        std::bind(&HttpResponse::completed, response));
  });

  try {
    server.start();
    scheduler.runLoop();
    server.stop();
  } catch (...) {
  }
  return 0;
}
