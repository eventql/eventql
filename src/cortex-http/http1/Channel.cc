// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-http/http1/Channel.h>
#include <cortex-http/http1/Connection.h>
#include <cortex-http/HttpTransport.h>
#include <cortex-http/HttpRequest.h>
#include <cortex-http/HttpResponse.h>
#include <cortex-base/Tokenizer.h>
#include <cortex-base/RuntimeError.h>

namespace cortex {
namespace http {
namespace http1 {

Channel::Channel(Connection* transport,
                 const HttpHandler& handler,
                 std::unique_ptr<HttpInput>&& input,
                 size_t maxRequestUriLength,
                 size_t maxRequestBodyLength,
                 HttpDateGenerator* dateGenerator,
                 HttpOutputCompressor* outputCompressor)
    : HttpChannel(transport, handler, std::move(input),
                  maxRequestUriLength, maxRequestBodyLength,
                  dateGenerator, outputCompressor),
      persistent_(false),
      connectionOptions_() {
}

Channel::~Channel() {
}

void Channel::reset() {
  connectionOptions_.clear();
  HttpChannel::reset();
}

void Channel::onMessageBegin(const BufferRef& method,
                                 const BufferRef& entity,
                                 HttpVersion version) {
  request_->setBytesReceived(bytesReceived());

  switch (version) {
    case HttpVersion::VERSION_1_1:
      persistent_ = true;
      break;
    case HttpVersion::VERSION_1_0:
    case HttpVersion::VERSION_0_9:
      persistent_ = false;
      break;
    default:
      RAISE(IllegalStateError, "Invalid State. Illegal version passed.");
  }

  HttpChannel::onMessageBegin(method, entity, version);
}

size_t Channel::bytesReceived() const noexcept {
  return static_cast<Connection*>(transport_)->bytesReceived();
}

void Channel::onMessageHeader(const BufferRef& name,
                                  const BufferRef& value) {
  request_->setBytesReceived(bytesReceived());

  if (!iequals(name, "Connection")) {
    HttpChannel::onMessageHeader(name, value);
    return;
  }

  std::vector<BufferRef> options = Tokenizer<BufferRef>::tokenize(value, ", ");

  for (const BufferRef& option: options) {
    connectionOptions_.push_back(option.str());

    if (iequals(option, "Keep-Alive"))
      persistent_ = true;
    else if (iequals(option, "close"))
      persistent_ = false;
  }
}

void Channel::onMessageHeaderEnd() {
  request_->setBytesReceived(bytesReceived());

  // hide transport-level header fields
  request_->headers().remove("Connection");
  for (const auto& name: connectionOptions_)
    request_->headers().remove(name);

  HttpChannel::onMessageHeaderEnd();
}

void Channel::onProtocolError(HttpStatus code, const std::string& message) {
  request_->setBytesReceived(bytesReceived());

  if (!response_->isCommitted()) {
    persistent_ = false;

    if (request_->version() != HttpVersion::UNKNOWN)
      response_->setVersion(request_->version());
    else
      response_->setVersion(HttpVersion::VERSION_0_9);

    response_->sendError(code, message);
  } else {
    transport_->abort();
  }
}

} // namespace http1
} // namespace http
} // namespace cortex
