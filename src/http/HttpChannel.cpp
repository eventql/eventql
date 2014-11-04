// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/http/HttpChannel.h>
#include <xzero/http/HttpTransport.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpResponseInfo.h>
#include <xzero/http/HttpOutput.h>
#include <xzero/http/HttpOutputCompressor.h>
#include <xzero/http/HttpVersion.h>
#include <xzero/http/HttpInputListener.h>
#include <xzero/http/BadMessage.h>
#include <xzero/logging/LogSource.h>
#include <xzero/io/FileRef.h>
#include <xzero/io/Filter.h>
#include <xzero/sysconfig.h>

namespace xzero {

static LogSource connectionLogger("http.HttpChannel");
#define ERROR(msg...) do { connectionLogger.error(msg); } while (0)
#ifndef NDEBUG
#define TRACE(msg...) do { connectionLogger.trace(msg); } while (0)
#else
#define TRACE(msg...) do {} while (0)
#endif

HttpChannel::HttpChannel(HttpTransport* transport, const HttpHandler& handler,
                         std::unique_ptr<HttpInput>&& input,
                         size_t maxRequestUriLength,
                         size_t maxRequestBodyLength,
                         HttpOutputCompressor* outputCompressor)
    : maxRequestUriLength_(maxRequestUriLength),
      maxRequestBodyLength_(maxRequestBodyLength),
      state_(HttpChannelState::IDLE),
      transport_(transport),
      request_(new HttpRequest(std::move(input))),
      response_(new HttpResponse(this, createOutput())),
      outputFilters_(),
      outputCompressor_(outputCompressor),
      handler_(handler) {
  //.
}

HttpChannel::~HttpChannel() {
  //.
}

void HttpChannel::reset() {
  state_ = HttpChannelState::IDLE;
  request_->recycle();
  response_->recycle();
  outputFilters_.clear();
}

std::unique_ptr<HttpOutput> HttpChannel::createOutput() {
  return std::unique_ptr<HttpOutput>(new HttpOutput(this));
}

void HttpChannel::addOutputFilter(std::shared_ptr<Filter> filter) {
  if (response()->isCommitted())
    throw std::runtime_error("Invalid State. Cannot add output filters after commit.");

  outputFilters_.push_back(filter);
}

void HttpChannel::removeAllOutputFilters() {
  if (response()->isCommitted())
    throw std::runtime_error("Invalid State. Cannot clear output filters after commit.");

  outputFilters_.clear();
}

void HttpChannel::send(const BufferRef& data, CompletionHandler&& onComplete) {
  onBeforeSend();

  if (outputFilters_.empty()) {
    if (!response_->isCommitted()) {
      HttpResponseInfo info(commitInline());
      transport_->send(std::move(info), data, std::move(onComplete));
    } else {
      transport_->send(data, std::move(onComplete));
    }
  } else {
    Buffer filtered;
    Filter::applyFilters(outputFilters_, data, &filtered, false);

    if (!response_->isCommitted()) {
      HttpResponseInfo info(commitInline());
      transport_->send(std::move(info), std::move(filtered), std::move(onComplete));
    } else {
      transport_->send(std::move(filtered), std::move(onComplete));
    }
  }
}

void HttpChannel::send(Buffer&& data, CompletionHandler&& onComplete) {
  onBeforeSend();

  if (!outputFilters_.empty()) {
    Buffer output;
    Filter::applyFilters(outputFilters_, data.ref(), &output, false);
    data = std::move(output);
  }

  if (!response_->isCommitted()) {
    HttpResponseInfo info(commitInline());
    transport_->send(std::move(info), std::move(data), std::move(onComplete));
  } else {
    transport_->send(std::move(data), std::move(onComplete));
  }
}

void HttpChannel::send(FileRef&& file, CompletionHandler&& onComplete) {
  onBeforeSend();

  if (outputFilters_.empty()) {
    if (!response_->isCommitted()) {
      HttpResponseInfo info(commitInline());
      transport_->send(std::move(info), BufferRef(), nullptr);
    }

    transport_->send(std::move(file), std::move(onComplete));
  } else {
    Buffer filtered;
    Filter::applyFilters(outputFilters_, file, &filtered, false);

    if (!response_->isCommitted()) {
      HttpResponseInfo info(commitInline());
      transport_->send(std::move(info), std::move(filtered), std::move(onComplete));
    } else {
      transport_->send(std::move(filtered), std::move(onComplete));
    }
  }
}

void HttpChannel::onBeforeSend() {
  // install some last-minute output filters before committing

  if (response_->isCommitted())
    return;

  // for (HttpChannelListener* listener: listeners_)
  //   listener->onBeforeCommit(request(), response());

  if (outputCompressor_)
    outputCompressor_->postProcess(request(), response());
}

HttpResponseInfo HttpChannel::commitInline() {
  if (!response_->status())
    throw std::runtime_error("No HTTP response status set yet.");

  if (request_->expect100Continue())
    response_->send100Continue();

  response_->setCommitted(true);

  const bool isHeadReq = request_->method() == HttpMethod::HEAD;
  HttpResponseInfo info(response_->version(), response_->status(),
                        response_->reason(), isHeadReq,
                        response_->contentLength(),
                        response_->headers(),
                        response_->trailers());

  if (!info.headers().contains("Server"))
    info.headers().push_back("Server", "xzero/" LIBXZERO_VERSION);

  return info;
}

void HttpChannel::commit(CompletionHandler&& onComplete) {
  TRACE("commit()");
  send(BufferRef(), std::move(onComplete));
}

void HttpChannel::send100Continue() {
  if (!request()->expect100Continue())
    throw std::runtime_error("Illegal State. no 100-continue expected.");

  request()->setExpect100Continue(false);

  HttpResponseInfo info(request_->version(), HttpStatus::ContinueRequest,
                        "Continue", false, 0, {}, {});

  TRACE("send100Continue(): sending it");
  transport_->send(std::move(info), BufferRef(), nullptr);
}

bool HttpChannel::onMessageBegin(const BufferRef& method,
                                 const BufferRef& entity,
                                 HttpVersion version) {
  response_->setVersion(version);
  request_->setVersion(version);
  request_->setMethod(method.str());
  if (!request_->setUri(entity.str())) {
    state_ = HttpChannelState::HANDLING;
    response_->sendError(HttpStatus::BadRequest);
    return false;
  }

  TRACE("onMessageBegin(%s, %s, %s)", request_->unparsedMethod().c_str(),
        request_->path().c_str(), to_string(version).c_str());

  return true;
}

bool HttpChannel::onMessageHeader(const BufferRef& name,
                                  const BufferRef& value) {
  request_->headers().push_back(name.str(), value.str());

  if (iequals(name, "Expect") && iequals(value, "100-continue"))
    request_->setExpect100Continue(true);

  // rfc7230, Section 5.4, p2
  if (iequals(name, "Host")) {
    if (request_->host().empty())
      request_->setHost(value.str());
    else
      throw BadMessage(HttpStatus::BadRequest, "Multiple host headers are illegal.");
  }

  return true;
}

bool HttpChannel::onMessageHeaderEnd() {
  if (state_ != HttpChannelState::HANDLING) {
    state_ = HttpChannelState::HANDLING;

    // rfc7230, Section 5.4, p2
    if (request_->version() == HttpVersion::VERSION_1_1) {
      if (!request_->headers().contains("Host")) {
        throw BadMessage(HttpStatus::BadRequest, "No Host header given.");
      }
    }

    handleRequest();
  }
  return true;
}

void HttpChannel::handleRequest() {
  try {
    handler_(request(), response());
  } catch (std::exception& e) {
    // TODO: reportException(e);
    response()->sendError(HttpStatus::InternalServerError, e.what());
  } catch (...) {
    // TODO: reportException(std::runtime_error("Unhandled unknown exception caught");
    response()->sendError(HttpStatus::InternalServerError,
                          "unhandled unknown exception");
  }
}

bool HttpChannel::onMessageContent(const BufferRef& chunk) {
  request_->input()->onContent(chunk);
  return true;
}

bool HttpChannel::onMessageEnd() {
  if (!request_->input())
    throw std::runtime_error("Internal Error. No HttpInput available?");

  if (request_->input()->listener())
    request_->input()->listener()->onAllDataRead();

  return false;
}

void HttpChannel::onProtocolError(HttpStatus code, const std::string& message) {
  TRACE("onProtocolError()");
  response_->sendError(code, message);
}

void HttpChannel::completed() {
  if (!response_->isCommitted()) {
    TRACE("completed(): not committed yet. commit empty-body response");
    if (!response_->hasContentLength() && request_->method() != HttpMethod::HEAD) {
      response_->setContentLength(0);
    }
    send(BufferRef(), std::bind(&HttpChannel::completed, this));
    return;
  }

  if (response_->hasContentLength() && response_->output()->size() < response_->contentLength()) {
    transport_->abort();
    return;
  }

  if (!outputFilters_.empty()) {
    Buffer filtered;
    Filter::applyFilters(outputFilters_, "", &filtered, true);
    transport_->send(std::move(filtered),
                     std::bind(&HttpTransport::completed, transport_));
    return;
  }

  transport_->completed();
}

}  // namespace xzero
