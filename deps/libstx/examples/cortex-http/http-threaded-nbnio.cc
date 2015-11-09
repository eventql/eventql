// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

/*
 * XXX demo: MTB request handler with STNB I/O.
 */

#include <cortex-base/executor/ThreadPool.h>
#include <cortex-base/executor/NativeScheduler.h>
#include <cortex-base/net/Server.h>
#include <cortex-base/net/InetConnector.h>
#include <cortex-base/WallClock.h>
#include <cortex-base/TimeSpan.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-http/HttpRequest.h>
#include <cortex-http/HttpResponse.h>
#include <cortex-http/HttpOutput.h>
#include <cortex-http/http1/ConnectionFactory.h>
#include <unistd.h>

using namespace cortex;
using namespace cortex::http;

void runJob(HttpRequest* request, HttpResponse* response, Executor* context) {
  timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = TimeSpan(0.1).nanoseconds();
  for (;;) {
    printf("request. job: nanosleep()ing %li\n", ts.tv_nsec);
    if (nanosleep(&ts, &ts) == 0 || errno != EINTR)
      break;
  }

  // run the complex stuff here
  BufferRef body = "Hello, World\n";

  // now respond to the client
  context->execute([=]() {
    printf("request. response\n");
    response->setStatus(HttpStatus::Ok);
    response->setContentLength(body.size());

    response->output()->write(body, std::bind(&HttpResponse::completed,
                                              response));
  });
}

int main() {
  NativeScheduler scheduler;
  WallClock* clock = WallClock::monotonic();

  //ThreadPool threaded(16);
  Server server;
  bool shutdown = false;

  auto inet = server.addConnector<InetConnector>(
      "http", &scheduler, &scheduler, clock,
      TimeSpan::fromSeconds(30),
      TimeSpan::fromSeconds(30),
      TimeSpan::Zero,
      &logAndPass,
      IPAddress("0.0.0.0"), 3000, 128, true, false);

  auto http = inet->addConnectionFactory<http1::ConnectionFactory>(
      clock, 100, 512, 5, TimeSpan::fromMinutes(3));

  http->setHandler([&](HttpRequest* request, HttpResponse* response) {
    printf("request\n");
    scheduler.execute(std::bind(&runJob, request, response, &scheduler));
    //threaded.execute(std::bind(&runJob, request, response, &scheduler));
  });

  server.start();
  scheduler.runLoop();
  server.stop();
  return 0;
}
