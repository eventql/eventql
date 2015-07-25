// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/TimeSpan.h>
#include <cortex-base/net/EndPointWriter.h>
#include <cortex-http/HttpTransport.h>
#include <cortex-http/HttpHandler.h>
#include <cortex-http/http1/Parser.h>
#include <cortex-http/http1/Generator.h>
#include <memory>

namespace cortex {
namespace http {

class HttpDateGenerator;
class HttpOutputCompressor;

namespace http1 {

class Channel;

/**
 * @brief Implements a HTTP/1.1 transport connection.
 */
class CORTEX_HTTP_API Connection : public ::cortex::Connection,
                                   public HttpTransport {
 public:
  Connection(EndPoint* endpoint,
                 Executor* executor,
                 const HttpHandler& handler,
                 HttpDateGenerator* dateGenerator,
                 HttpOutputCompressor* outputCompressor,
                 size_t maxRequestUriLength,
                 size_t maxRequestBodyLength,
                 size_t maxRequestCount,
                 TimeSpan maxKeepAlive);
  ~Connection();

  size_t bytesReceived() const noexcept { return parser_.bytesReceived(); }
  size_t bytesTransmitted() const noexcept { return generator_.bytesTransmitted(); }

  // HttpTransport overrides
  void abort() override;
  void completed() override;
  void send(HttpResponseInfo&& responseInfo, Buffer&& chunk,
            CompletionHandler onComplete) override;
  void send(HttpResponseInfo&& responseInfo, const BufferRef& chunk,
            CompletionHandler onComplete) override;
  void send(HttpResponseInfo&& responseInfo, FileRef&& chunk,
            CompletionHandler onComplete) override;
  void send(Buffer&& chunk, CompletionHandler onComplete) override;
  void send(const BufferRef& chunk, CompletionHandler onComplete) override;
  void send(FileRef&& chunk, CompletionHandler onComplete) override;

 private:
  void patchResponseInfo(HttpResponseInfo& info);
  void parseFragment();
  void onResponseComplete(bool succeed);

  // Connection overrides
  void onOpen() override;
  void onClose() override;
  void setInputBufferSize(size_t size) override;
  void onFillable() override;
  void onFlushable() override;
  void onInterestFailure(const std::exception& error) override;

 private:
  Parser parser_;

  Buffer inputBuffer_;
  size_t inputOffset_;

  EndPointWriter writer_;
  CompletionHandler onComplete_;
  Generator generator_;

  std::unique_ptr<Channel> channel_;
  TimeSpan maxKeepAlive_;
  size_t requestCount_;
  size_t requestMax_;
};

}  // namespace http1
}  // namespace http
}  // namespace cortex
