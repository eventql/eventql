#include <xzero/http/v1/HttpConnection.h>
#include <xzero/http/v1/Http1Channel.h>
#include <xzero/http/v1/Http1Input.h>
#include <xzero/http/HttpDateGenerator.h>
#include <xzero/http/HttpResponseInfo.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/BadMessage.h>
#include <xzero/net/Connection.h>
#include <xzero/net/EndPoint.h>
#include <xzero/net/EndPointWriter.h>
#include <xzero/logging/LogSource.h>
#include <xzero/WallClock.h>
#include <xzero/sysconfig.h>
#include <cassert>
#include <cstdlib>

namespace xzero {
namespace http1 {

static LogSource connectionLogger("http1.HttpConnection");
#define ERROR(msg...) do { connectionLogger.error(msg); } while (0)
#ifndef NDEBUG
#define TRACE(msg...) do { connectionLogger.trace(msg); } while (0)
#else
#define TRACE(msg...) do {} while (0)
#endif

HttpConnection::HttpConnection(std::shared_ptr<EndPoint> endpoint,
                               const HttpHandler& handler,
                               HttpDateGenerator* dateGenerator,
                               HttpOutputCompressor* outputCompressor,
                               size_t maxRequestUriLength,
                               size_t maxRequestBodyLength,
                               size_t maxRequestCount,
                               TimeSpan maxKeepAlive)
    : HttpTransport(endpoint),
      parser_(HttpParser::REQUEST),
      inputBuffer_(),
      inputOffset_(0),
      writer_(),
      onComplete_(),
      generator_(dateGenerator, &writer_),
      channel_(new Http1Channel(
          this, handler, std::unique_ptr<HttpInput>(new Http1Input(this)),
          maxRequestUriLength, maxRequestBodyLength, outputCompressor)),
      maxKeepAlive_(maxKeepAlive),
      requestCount_(0),
      requestMax_(maxRequestCount) {

  parser_.setListener(channel_.get());
  TRACE("%p ctor", this);
}

HttpConnection::~HttpConnection() {
  TRACE("%p dtor", this);
}

void HttpConnection::onOpen() {
  TRACE("%p onOpen", this);
  HttpTransport::onOpen();

  // TODO support TCP_DEFER_ACCEPT here
#if 0
  if (connector()->deferAccept())
    onFillable();
  else
    wantFill();
#else
  wantFill();
#endif
}

void HttpConnection::onClose() {
  TRACE("%p onClose", this);
  HttpTransport::onClose();
}

void HttpConnection::abort() {
  TRACE("%p abort", this);
  endpoint()->close();
}

void HttpConnection::completed() {
  TRACE("%p completed", this);

  if (onComplete_)
    throw std::runtime_error("there is still another completion hook.");

  if (!generator_.isChunked() && generator_.pendingContentLength() > 0)
    throw std::runtime_error(
        "Invalid State. Response not fully written but completed() invoked.");

  generator_.generateTrailer(channel_->response()->trailers());

  onComplete_ = [this](bool) {
    if (channel_->isPersistent()) {
      TRACE("%p completed.onComplete", this);
      // re-use on keep-alive
      channel_->reset();

      if (endpoint()->isCorking()) {
        endpoint()->setCorking(false);
      }

      if (inputOffset_ < inputBuffer_.size()) {
        TRACE("%p completed.onComplete: pipelined read", this);
        // have some request pipelined
        onFillable();
      } else {
        TRACE("%p completed.onComplete: keep-alive read", this);
        // wait for next request
        wantFill();
      }
    } else {
      endpoint()->close();
    }
  };

  wantFlush();
}

void HttpConnection::send(HttpResponseInfo&& responseInfo,
                          const BufferRef& chunk,
                          CompletionHandler&& onComplete) {
  if (onComplete && onComplete_)
    throw std::runtime_error("there is still another completion hook.");

  TRACE("%p send(status=%d, persistent=%s, chunkSize=%zu)",
        this, responseInfo.status(), channel_->isPersistent() ? "yes" : "no",
        chunk.size());

  patchResponseInfo(responseInfo);

  const bool corking_ = true;  // TODO(TCP_CORK): part of HttpResponseInfo?
  if (corking_)
    endpoint()->setCorking(true);

  generator_.generateResponse(responseInfo, chunk);
  onComplete_ = onComplete;
  wantFlush();
}

void HttpConnection::send(HttpResponseInfo&& responseInfo,
                          Buffer&& chunk,
                          CompletionHandler&& onComplete) {
  if (onComplete && onComplete_)
    throw std::runtime_error("there is still another completion hook.");

  TRACE("%p send(status=%d, persistent=%s, chunkSize=%zu)",
        this, responseInfo.status(), channel_->isPersistent() ? "yes" : "no",
        chunk.size());

  patchResponseInfo(responseInfo);

  const bool corking_ = true;  // TODO(TCP_CORK): part of HttpResponseInfo?
  if (corking_)
    endpoint()->setCorking(true);

  generator_.generateResponse(responseInfo, std::move(chunk));
  onComplete_ = onComplete;
  wantFlush();
}

void HttpConnection::send(HttpResponseInfo&& responseInfo,
                          FileRef&& chunk,
                          CompletionHandler&& onComplete) {
  if (onComplete && onComplete_)
    throw std::runtime_error("there is still another completion hook.");

  TRACE("%p send(status=%d, persistent=%s, fileRef.fd=%d, chunkSize=%zu)",
        this, responseInfo.status(), channel_->isPersistent() ? "yes" : "no",
        chunk.handle(), chunk.size());

  patchResponseInfo(responseInfo);

  const bool corking_ = true;  // TODO(TCP_CORK): part of HttpResponseInfo?
  if (corking_)
    endpoint()->setCorking(true);

  generator_.generateResponse(responseInfo, std::move(chunk));
  onComplete_ = onComplete;
  wantFlush();
}

void HttpConnection::patchResponseInfo(HttpResponseInfo& responseInfo) {
  if (static_cast<int>(responseInfo.status()) >= 200) {
    // patch in HTTP transport-layer headers
    if (channel_->isPersistent() && requestCount_ < requestMax_) {
      ++requestCount_;

      char keepAlive[64];
      snprintf(keepAlive, sizeof(keepAlive), "timeout=%lu, max=%zu",
               maxKeepAlive_.totalSeconds(), requestMax_ - requestCount_);

      responseInfo.headers().push_back("Connection", "Keep-Alive");
      responseInfo.headers().push_back("Keep-Alive", keepAlive);
    } else {
      channel_->setPersistent(false);
      responseInfo.headers().push_back("Connection", "closed");
    }
  }
}

void HttpConnection::send(Buffer&& chunk,
                          CompletionHandler&& onComplete) {
  if (onComplete && onComplete_)
    throw std::runtime_error("there is still another completion hook.");

  TRACE("%p send(chunkSize=%zu)", this, chunk.size());

  generator_.generateBody(std::move(chunk));
  onComplete_ = std::move(onComplete);

  wantFlush();
}

void HttpConnection::send(const BufferRef& chunk,
                          CompletionHandler&& onComplete) {
  if (onComplete && onComplete_)
    throw std::runtime_error("there is still another completion hook.");

  TRACE("%p send(chunkSize=%zu)", this, chunk.size());

  generator_.generateBody(chunk);
  onComplete_ = std::move(onComplete);

  wantFlush();
}

void HttpConnection::send(FileRef&& chunk, CompletionHandler&& onComplete) {
  if (onComplete && onComplete_)
    throw std::runtime_error("there is still another completion hook.");

  TRACE("%p send(chunkSize=%zu)", this, chunk.size());

  generator_.generateBody(std::move(chunk));
  onComplete_ = std::move(onComplete);
  wantFlush();
}

void HttpConnection::setInputBufferSize(size_t size) {
  inputBuffer_.reserve(size);
}

void HttpConnection::onFillable() {
  TRACE("%p onFillable", this);

  if (endpoint()->fill(&inputBuffer_) == 0) {
    abort();  // throw std::runtime_error("client EOF");
  }

  try {
    size_t n = parser_.parseFragment(inputBuffer_.ref(inputOffset_));
    inputOffset_ += n;
  } catch (const BadMessage& e) {
    TRACE("%p onFillable: BadMessage caught. %s", this, e.what());
    channel_->response()->sendError(e.code(), e.what());
  }
}

void HttpConnection::onFlushable() {
  TRACE("%p onFlushable", this);

  const bool complete = writer_.flush(endpoint());
  if (complete) {
    wantFill(); // restore interest to NONE or READ

    if (onComplete_) {
      TRACE("%p onFlushable: invoking completion callback", this);
      auto callback = std::move(onComplete_);
      callback(true);
    }
  }
}

void HttpConnection::onInterestFailure(const std::exception& error) {
  ERROR("onInterestFailure. %s", error.what());
  //printf("onInterestFailure. %s\n", error.what());

  if (!channel_->response()->isCommitted()) {
    ERROR("onInterestFailure. no response comitted yet, sending 500 then");
    channel_->setPersistent(false);
    channel_->response()->sendError(HttpStatus::InternalServerError,
                                    error.what());
  } else {
    // TODO: improve logging here, as this eats our exception here.
    // e.g. via (factory or connector)->report(this, error);
    abort();
  }
}

}  // namespace http1
}  // namespace xzero
