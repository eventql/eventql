// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-base/executor/ThreadedExecutor.h>
#include <cortex-base/net/Server.h>
#include <cortex-base/net/InetConnector.h>
#include <cortex-base/WallClock.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-http/HttpRequest.h>
#include <cortex-http/HttpResponse.h>
#include <cortex-http/HttpOutput.h>
#include <cortex-http/http1/ConnectionFactory.h>
#include <unistd.h>

using namespace cortex;
using namespace cortex::http;

int main() {
  ThreadedExecutor threadedExecutor;
  Server server;
  WallClock* clock = WallClock::monotonic();
  bool shutdown = false;

  auto inet = server.addConnector<InetConnector>(
      "http", &threadedExecutor, nullptr, nullptr, clock,
      TimeSpan::fromSeconds(30),
      TimeSpan::Zero,
      &logAndPass,
      IPAddress("0.0.0.0"), 3000, 128, true, false);
  inet->setBlocking(true);

  auto http = inet->addConnectionFactory<http1::ConnectionFactory>(
      clock, 100, 512, 5, TimeSpan::fromMinutes(3));

  http->setHandler([](HttpRequest* request, HttpResponse* response) {
    BufferRef body = "Hello, World\n";

    response->setStatus(HttpStatus::Ok);
    response->setContentLength(body.size());

    response->output()->write(body, std::bind(&HttpResponse::completed,
                                              response));
  });

  server.start();

  while (!shutdown)
    sleep(1);

  server.stop();

  return 0;
}
