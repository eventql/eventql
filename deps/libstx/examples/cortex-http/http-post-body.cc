// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-base/executor/DirectExecutor.h>
#include <cortex-base/executor/NativeScheduler.h>
#include <cortex-base/WallClock.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/net/Server.h>
#include <cortex-base/net/InetConnector.h>
#include <cortex-base/logging/LogTarget.h>
#include <cortex-base/logging/LogAggregator.h>
#include <cortex-http/HttpRequest.h>
#include <cortex-http/HttpResponse.h>
#include <cortex-http/HttpOutput.h>
#include <cortex-http/HttpInput.h>
#include <cortex-http/HttpInputListener.h>
#include <cortex-http/http1/ConnectionFactory.h>
#include <vector>
#include <string>

using namespace cortex;
using namespace cortex::http;

class HttpEcho : public HttpInputListener {
 public:
  HttpEcho(HttpRequest* req, HttpResponse* resp)
      : request_(req), response_(resp), bodyChunk_()
  {
    printf("going\n");
    if (request_->expect100Continue())
      response_->send100Continue(std::bind(&HttpEcho::go, this));
    else
      go();
  }

  void go() {
    printf("go\n");
    request_->input()->setListener(this);
    response_->setStatus(HttpStatus::Ok);

    for (const HeaderField& field : request_->headers()) {
      printf("[Header] %s: %s\n", field.name().c_str(), field.value().c_str());
    }
  }

  void onContentAvailable() override {
    printf("onContentAvailable\n");
    request_->input()->read(&bodyChunk_);
    response_->output()->write(bodyChunk_);
    bodyChunk_.clear();
  }

  void onAllDataRead() override {
    printf("onAllDataRead ;-)\n");
    response_->completed();
    delete this;
  }

  void onError(const std::string& errorMessage) override {
    printf("onError: %s\n", errorMessage.c_str());
    response_->completed();
    delete this;
  }

 private:
  HttpRequest* request_;
  HttpResponse* response_;
  Buffer bodyChunk_;
};

int main() {
  LogAggregator::get().setLogTarget(LogTarget::console());
  LogAggregator::get().setLogLevel(LogLevel::Trace);

  NativeScheduler scheduler;
  WallClock* clock = WallClock::monotonic();

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
    new HttpEcho(request, response);
  });

  server.start();
  scheduler.runLoop();
  server.stop();
  return 0;
}
