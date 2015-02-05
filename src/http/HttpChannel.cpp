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
#include <xzero/RuntimeError.h>
#include <xzero/sysconfig.h>

namespace xzero {

static LogSource connectionLogger("http.HttpChannel");
#define ERROR(msg...) do { connectionLogger.error(msg); } while (0)
#ifndef NDEBUG
#define TRACE(msg...) do { connectionLogger.trace(msg); } while (0)
#else
#define TRACE(msg...) do {} while (0)
#endif

std::string to_string(HttpChannelState state) {
  switch (state) {
    case HttpChannelState::READING: return "READING";
    case HttpChannelState::HANDLING: return "HANDLING";
    case HttpChannelState::SENDING: return "SENDING";
    case HttpChannelState::DONE: return "DONE";
    default: {
      char buf[128];
      int n = snprintf(buf, sizeof(buf), "<%d>", static_cast<int>(state));
      return std::string(buf, n);
    }
  }
}

HttpChannel::HttpChannel(HttpTransport* transport, const HttpHandler& handler,
                         std::unique_ptr<HttpInput>&& input,
                         size_t maxRequestUriLength,
                         size_t maxRequestBodyLength,
                         HttpOutputCompressor* outputCompressor)
    : maxRequestUriLength_(maxRequestUriLength),
      maxRequestBodyLength_(maxRequestBodyLength),
      state_(HttpChannelState::READING),
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
  setState(HttpChannelState::READING);
  request_->recycle();
  response_->recycle();
  outputFilters_.clear();
}

void HttpChannel::setState(HttpChannelState newState) {
  TRACE("%p setState from %s to %s",
        this,
        to_string(state_).c_str(),
        to_string(newState).c_str());

  state_ = newState;
}

CompletionHandler HttpChannel::makeCompleter(CompletionHandler next) {
  return next;

  // return [this, next](bool succeeded) {
  //   BUG_ON(state() != HttpChannelState::SENDING);
  //   setState(HttpChannelState::HANDLING);

  //   if (next) {
  //     next(succeeded);
  //   }
  // };
}

std::unique_ptr<HttpOutput> HttpChannel::createOutput() {
  return std::unique_ptr<HttpOutput>(new HttpOutput(this));
}

void HttpChannel::addOutputFilter(std::shared_ptr<Filter> filter) {
  if (response()->isCommitted())
    throw RUNTIME_ERROR("Invalid State. Cannot add output filters after commit.");

  outputFilters_.push_back(filter);
}

void HttpChannel::removeAllOutputFilters() {
  if (response()->isCommitted())
    throw RUNTIME_ERROR("Invalid State. Cannot clear output filters after commit.");

  outputFilters_.clear();
}

void HttpChannel::send(const BufferRef& data, CompletionHandler&& onComplete) {
  onBeforeSend();

  auto next = makeCompleter(onComplete);

  if (outputFilters_.empty()) {
    if (!response_->isCommitted()) {
      HttpResponseInfo info(commitInline());
      transport_->send(std::move(info), data, std::move(next));
    } else {
      transport_->send(data, std::move(next));
    }
  } else {
    Buffer filtered;
    Filter::applyFilters(outputFilters_, data, &filtered, false);

    if (!response_->isCommitted()) {
      HttpResponseInfo info(commitInline());
      transport_->send(std::move(info), std::move(filtered), std::move(next));
    } else {
      transport_->send(std::move(filtered), std::move(next));
    }
  }
}

void HttpChannel::send(Buffer&& data, CompletionHandler&& onComplete) {
  onBeforeSend();

  auto next = makeCompleter(onComplete);

  if (!outputFilters_.empty()) {
    Buffer output;
    Filter::applyFilters(outputFilters_, data.ref(), &output, false);
    data = std::move(output);
  }

  if (!response_->isCommitted()) {
    HttpResponseInfo info(commitInline());
    transport_->send(std::move(info), std::move(data), std::move(next));
  } else {
    transport_->send(std::move(data), std::move(next));
  }
}

void HttpChannel::send(FileRef&& file, CompletionHandler&& onComplete) {
  onBeforeSend();

  auto next = makeCompleter(onComplete);

  if (outputFilters_.empty()) {
    if (!response_->isCommitted()) {
      HttpResponseInfo info(commitInline());
      transport_->send(std::move(info), BufferRef(), nullptr);
      transport_->send(std::move(file), std::move(next));
      // transport_->send(std::move(info), BufferRef(),
      //     std::bind(&HttpTransport::send, transport_, file, next));
    } else {
      transport_->send(std::move(file), std::move(next));
    }
  } else {
    Buffer filtered;
    Filter::applyFilters(outputFilters_, file, &filtered, false);

    if (!response_->isCommitted()) {
      HttpResponseInfo info(commitInline());
      transport_->send(std::move(info), std::move(filtered), std::move(next));
    } else {
      transport_->send(std::move(filtered), std::move(next));
    }
  }
}

void HttpChannel::onBeforeSend() {
  // also accept READING as current state because you
  // might want to sentError right away, due to protocol error at least.

  if (state() != HttpChannelState::HANDLING &&
      state() != HttpChannelState::READING) {
    throw RUNTIME_ERROR("Invalid state (" +
        to_string(state()) + ". Creating a new send object not allowed.");
  }

  //XXX setState(HttpChannelState::SENDING);

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
    throw RUNTIME_ERROR("No HTTP response status set yet.");

  if (request_->expect100Continue())
    response_->send100Continue(nullptr /* FIXME */);

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
  send(BufferRef(), makeCompleter(onComplete));
}

void HttpChannel::send100Continue(CompletionHandler&& onComplete) {
  if (!request()->expect100Continue())
    throw RUNTIME_ERROR("Illegal State. no 100-continue expected.");

  request()->setExpect100Continue(false);

  auto next = makeCompleter(onComplete);

  HttpResponseInfo info(request_->version(), HttpStatus::ContinueRequest,
                        "Continue", false, 0, {}, {});

  TRACE("send100Continue(): sending it");
  transport_->send(std::move(info), BufferRef(), std::move(next));
}

bool HttpChannel::onMessageBegin(const BufferRef& method,
                                 const BufferRef& entity,
                                 HttpVersion version) {
  response_->setVersion(version);
  request_->setVersion(version);
  request_->setMethod(method.str());
  if (!request_->setUri(entity.str())) {
    setState(HttpChannelState::HANDLING);
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
    else {
      setState(HttpChannelState::HANDLING);
      throw BadMessage(HttpStatus::BadRequest, "Multiple host headers are illegal.");
    }
  }

  return true;
}

bool HttpChannel::onMessageHeaderEnd() {
  if (state() != HttpChannelState::HANDLING) {
    setState(HttpChannelState::HANDLING);

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
    consoleLogger(e);
    response()->sendError(HttpStatus::InternalServerError, e.what());
  } catch (...) {
    // TODO: reportException(RUNTIME_ERROR("Unhandled unknown exception caught");
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
    throw RUNTIME_ERROR("Internal Error. No HttpInput available?");

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

  if (state() != HttpChannelState::HANDLING) {
    throw RUNTIME_ERROR("Invalid HTTP channel state " +
        to_string(state()) + ". Expected " +
        to_string(HttpChannelState::HANDLING) + ".");
  }

  setState(HttpChannelState::DONE);

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
