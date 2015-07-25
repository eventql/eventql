// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-http/HttpChannel.h>
#include <cortex-http/HttpTransport.h>
#include <cortex-http/HttpRequest.h>
#include <cortex-http/HttpResponse.h>
#include <cortex-http/HttpResponseInfo.h>
#include <cortex-http/HttpDateGenerator.h>
#include <cortex-http/HttpOutput.h>
#include <cortex-http/HttpOutputCompressor.h>
#include <cortex-http/HttpVersion.h>
#include <cortex-http/HttpInputListener.h>
#include <cortex-http/BadMessage.h>
#include <cortex-http/sysconfig.h>
#include <cortex-base/logging.h>
#include <cortex-base/io/FileRef.h>
#include <cortex-base/io/Filter.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/sysconfig.h>

namespace cortex {
namespace http {

#define ERROR(msg...) logError("http.HttpChannel", msg)

#ifndef NDEBUG
#define TRACE(msg...) logTrace("http.HttpChannel", msg)
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
                         HttpDateGenerator* dateGenerator,
                         HttpOutputCompressor* outputCompressor)
    : maxRequestUriLength_(maxRequestUriLength),
      maxRequestBodyLength_(maxRequestBodyLength),
      state_(HttpChannelState::READING),
      transport_(transport),
      request_(new HttpRequest(std::move(input))),
      response_(new HttpResponse(this, createOutput())),
      dateGenerator_(dateGenerator),
      outputFilters_(),
      outputCompressor_(outputCompressor),
      handler_(handler) {
  //.
}

HttpChannel::~HttpChannel() {
  TRACE("%p dtor", this);
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

std::unique_ptr<HttpOutput> HttpChannel::createOutput() {
  return std::unique_ptr<HttpOutput>(new HttpOutput(this));
}

void HttpChannel::addOutputFilter(std::shared_ptr<Filter> filter) {
  if (response()->isCommitted())
    RAISE(IllegalStateError, "Invalid State. Cannot add output filters after commit.");

  outputFilters_.push_back(filter);
}

void HttpChannel::removeAllOutputFilters() {
  if (response()->isCommitted())
    RAISE(IllegalStateError, "Invalid State. Cannot add output filters after commit.");

  outputFilters_.clear();
}

void HttpChannel::send(const BufferRef& data, CompletionHandler onComplete) {
  onBeforeSend();

  if (outputFilters_.empty()) {
    if (!response_->isCommitted()) {
      HttpResponseInfo info(commitInline());
      transport_->send(std::move(info), data, onComplete);
    } else {
      transport_->send(data, onComplete);
    }
  } else {
    Buffer filtered;
    Filter::applyFilters(outputFilters_, data, &filtered, false);

    if (!response_->isCommitted()) {
      HttpResponseInfo info(commitInline());
      transport_->send(std::move(info), std::move(filtered), onComplete);
    } else {
      transport_->send(std::move(filtered), onComplete);
    }
  }
}

void HttpChannel::send(Buffer&& data, CompletionHandler onComplete) {
  onBeforeSend();

  if (!outputFilters_.empty()) {
    Buffer output;
    Filter::applyFilters(outputFilters_, data.ref(), &output, false);
    data = std::move(output);
  }

  if (!response_->isCommitted()) {
    HttpResponseInfo info(commitInline());
    transport_->send(std::move(info), std::move(data), onComplete);
  } else {
    transport_->send(std::move(data), onComplete);
  }
}

void HttpChannel::send(FileRef&& file, CompletionHandler onComplete) {
  onBeforeSend();

  if (outputFilters_.empty()) {
    if (!response_->isCommitted()) {
      HttpResponseInfo info(commitInline());
      transport_->send(std::move(info), BufferRef(), nullptr);
      transport_->send(std::move(file), onComplete);
      // transport_->send(std::move(info), BufferRef(),
      //     std::bind(&HttpTransport::send, transport_, file, onComplete));
    } else {
      transport_->send(std::move(file), onComplete);
    }
  } else {
    Buffer filtered;
    Filter::applyFilters(outputFilters_, file, &filtered, false);

    if (!response_->isCommitted()) {
      HttpResponseInfo info(commitInline());
      transport_->send(std::move(info), std::move(filtered), onComplete);
    } else {
      transport_->send(std::move(filtered), onComplete);
    }
  }
}

void HttpChannel::onBeforeSend() {
  // also accept READING as current state because you
  // might want to sentError right away, due to protocol error at least.

  if (state() != HttpChannelState::HANDLING &&
      state() != HttpChannelState::READING) {
    RAISE(IllegalStateError, "Invalid state (" + to_string(state()) + ". Creating a new send object not allowed.");
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
    RAISE(IllegalStateError, "No HTTP response status set yet.");

  onPostProcess_();

  if (request_->expect100Continue())
    response_->send100Continue(nullptr);

  response_->setCommitted(true);

  const bool isHeadReq = request_->method() == HttpMethod::HEAD;
  HttpResponseInfo info(response_->version(), response_->status(),
                        response_->reason(), isHeadReq,
                        response_->contentLength(),
                        response_->headers(),
                        response_->trailers());

  if (!info.headers().contains("Server"))
    info.headers().push_back("Server", "cortex-base/" CORTEX_HTTP_VERSION);

  if (dateGenerator_ && static_cast<int>(response_->status()) >= 200) {
    Buffer date;
    dateGenerator_->fill(&date);
    info.headers().push_back("Date", date.str());
  }

  return info;
}

void HttpChannel::commit(CompletionHandler onComplete) {
  TRACE("commit()");
  send(BufferRef(), onComplete);
}

void HttpChannel::send100Continue(CompletionHandler onComplete) {
  if (!request()->expect100Continue())
    RAISE(IllegalStateError, "Illegal State. no 100-continue expected.");

  request()->setExpect100Continue(false);

  HttpResponseInfo info(request_->version(), HttpStatus::ContinueRequest,
                        "Continue", false, 0, {}, {});

  TRACE("send100Continue(): sending it");
  transport_->send(std::move(info), BufferRef(), onComplete);
}

void HttpChannel::onMessageBegin(const BufferRef& method,
                                 const BufferRef& entity,
                                 HttpVersion version) {
  response_->setVersion(version);
  request_->setVersion(version);
  request_->setMethod(method.str());
  if (!request_->setUri(entity.str())) {
    RAISE_EXCEPTION(BadMessage, HttpStatus::BadRequest);
  }

  TRACE("onMessageBegin(%s, %s, %s)", request_->unparsedMethod().c_str(),
        request_->path().c_str(), to_string(version).c_str());
}

void HttpChannel::onMessageHeader(const BufferRef& name,
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
      RAISE_HTTP_REASON(BadRequest, "Multiple host headers are illegal.");
    }
  }
}

void HttpChannel::onMessageHeaderEnd() {
  if (state() != HttpChannelState::HANDLING) {
    setState(HttpChannelState::HANDLING);

    // rfc7230, Section 5.4, p2
    if (request_->version() == HttpVersion::VERSION_1_1) {
      if (!request_->headers().contains("Host")) {
        RAISE_HTTP_REASON(BadRequest, "No Host header given.");
      }
    }

    handleRequest();
  }
}

void HttpChannel::handleRequest() {
  try {
    handler_(request(), response());
  } catch (std::exception& e) {
    // TODO: reportException(e);
    logError("HttpChannel", e);
    response()->sendError(HttpStatus::InternalServerError, e.what());
  } catch (...) {
    // TODO: reportException(RUNTIME_ERROR("Unhandled unknown exception caught");
    response()->sendError(HttpStatus::InternalServerError,
                          "unhandled unknown exception");
  }
}

void HttpChannel::onMessageContent(const BufferRef& chunk) {
  request_->input()->onContent(chunk);
}

void HttpChannel::onMessageEnd() {
  BUG_ON(request_->input() == nullptr);

  if (request_->input()->listener())
    request_->input()->listener()->onAllDataRead();
}

void HttpChannel::onProtocolError(HttpStatus code, const std::string& message) {
  TRACE("onProtocolError()");
  response_->sendError(code, message);
}

void HttpChannel::completed() {
  TRACE("completed!");

  if (response_->hasContentLength() && response_->output()->size() < response_->contentLength()) {
    RAISE(RuntimeError,
          "Attempt to complete() a response before having written the full response body (%zu of %zu).",
          response_->output()->size(),
          response_->contentLength());
  }

  if (state() != HttpChannelState::HANDLING) {
    RAISE(IllegalStateError, "HttpChannel.completed invoked but state is not in HANDLING.");
  }

  if (!outputFilters_.empty()) {
    TRACE("%p completed: send(applyFilters(EOS))", this);
    Buffer filtered;
    Filter::applyFilters(outputFilters_, "", &filtered, true);
    transport_->send(std::move(filtered), nullptr);
  } else if (!response_->isCommitted()) {
    TRACE("%p completed: not committed yet. commit empty-body response", this);
    if (!response_->hasContentLength() && request_->method() != HttpMethod::HEAD) {
      response_->setContentLength(0);
    }
    HttpResponseInfo info(commitInline());
    transport_->send(std::move(info), BufferRef(), nullptr);
  }

  setState(HttpChannelState::DONE);

  TRACE("%p completed: pass on to transport layer", this);
  transport_->completed();
}

void HttpChannel::onPostProcess(std::function<void()> callback) {
  // TODO: ensure we can still add us
  onPostProcess_.connect(callback);
}

void HttpChannel::onResponseEnd(std::function<void()> callback) {
  onResponseEnd_.connect(callback);
}

void HttpChannel::responseEnd() {
  auto cb = std::move(onResponseEnd_);
  onResponseEnd_.clear();
  cb();
}

}  // namespace http
}  // namespace cortex
