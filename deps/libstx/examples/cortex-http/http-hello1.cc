// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-base/RuntimeError.h>
#include <cortex-base/WallClock.h>
#include <cortex-base/executor/NativeScheduler.h>
#include <cortex-base/executor/DirectExecutor.h>
#include <cortex-base/net/Server.h>
#include <cortex-base/net/InetConnector.h>
#include <cortex-base/net/SslConnector.h>
#include <cortex-http/HttpRequest.h>
#include <cortex-http/HttpResponse.h>
#include <cortex-http/HttpOutput.h>
#include <cortex-http/http1/ConnectionFactory.h>
#include <cortex-base/logging.h>

using namespace cortex;
using namespace cortex::http;

std::unique_ptr<SslConnector> createSslConnector( // {{{
    const std::string& name, int port, Executor* executor,
    Scheduler* scheduler, WallClock* clock) {

  std::unique_ptr<SslConnector> connector(
      new SslConnector(name, executor, scheduler, clock,
                              TimeSpan::fromSeconds(30),
                              TimeSpan::fromSeconds(30),
                              TimeSpan::Zero,
                              [](const std::exception& e) {
                                 logError("hello", e); },
                              IPAddress("0.0.0.0"), port, 128,
                              true, true));

  connector->addContext("../../server.crt", "../../server.key");

  return connector;
}
// }}}

int main() {
  auto clock = WallClock::monotonic();
  NativeScheduler scheduler;
  Server server;

  auto inet = server.addConnector<InetConnector>(
      "http", &scheduler, &scheduler,  clock,
      TimeSpan::fromSeconds(60),
      TimeSpan::fromSeconds(60),
      TimeSpan::Zero,
      &logAndPass,
      IPAddress("0.0.0.0"), 3000, 128, true, false);
  inet->setBlocking(false);

  auto https = createSslConnector("https", 3443, &scheduler, &scheduler, clock);

  auto http = inet->addConnectionFactory<http1::ConnectionFactory>(
      clock, 100, 512, 5, TimeSpan::fromMinutes(3));

  auto handler = [&](HttpRequest* request,
                     HttpResponse* response) {
    response->setStatus(HttpStatus::Ok);
    response->setReason("because");

    if (request->path() == "/bye") {
      server.stop();
    }

    Buffer body;
    body << "Hello " << request->path() << "\n";
    response->setContentLength(body.size());
    response->output()->write(
        std::move(body),
        std::bind(&HttpResponse::completed, response));
  };

  http->setHandler(handler);

  https->addConnectionFactory<http1::ConnectionFactory>(
      clock, 100, 512, 5, TimeSpan::fromMinutes(3))->setHandler(handler);

  server.addConnector(std::move(https));

  server.start();
  scheduler.runLoop();
  server.stop();

  return 0;
}
