#include <xzero/http/v1/HttpConnection.h>
#include <xzero/http/v1/HttpChannel.h>
#include <xzero/http/v1/HttpInput.h>
#include <xzero/http/HttpDateGenerator.h>
#include <xzero/http/HttpResponseInfo.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpRequest.h>
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
#ifndef NDEBUG
#define TRACE(msg...) do { connectionLogger.trace(msg); } while (0)
#else
#define TRACE(msg...) do {} while (0)
#endif

HttpConnection::HttpConnection(std::shared_ptr<EndPoint> endpoint,
                               const HttpHandler& handler,
                               HttpDateGenerator* dateGenerator)
    : HttpTransport(endpoint),
      parser_(HttpParser::REQUEST),
      generator_(dateGenerator),
      inputBuffer_(),
      inputOffset_(0),
      writer_(),
      onComplete_(),
      channel_(new HttpChannel(
          this, handler, std::unique_ptr<HttpInput>(new HttpInput(this)))),
      maxKeepAlive_(TimeSpan::fromSeconds(60)),  // TODO: parametrize me
      requestCount_(0),
      requestMax_(100) {

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

  generator_.generateBody(BufferRef(), true, &writer_);  // EOS

  onComplete_ = [this](bool) {
    if (channel_->isPersistent()) {
      TRACE("%p completed.onComplete", this);
      // re-use on keep-alive
      channel_->reset();

      if (endpoint()->isCorking()) endpoint()->setCorking(false);

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

  if (static_cast<int>(responseInfo.status()) >= 200) {
    // patch in HTTP transport-layer headers
    if (channel_->isPersistent() && requestCount_ < requestMax_) {
      ++requestCount_;

      char keepAlive[64];
      snprintf(keepAlive, sizeof(keepAlive), "timeout=%lu, max=%zu",
               maxKeepAlive_.totalSeconds(), requestMax_ - requestCount_);

      responseInfo.headers().push_back("Connection", "keep-alive");
      responseInfo.headers().push_back("Keep-Alive", keepAlive);
    } else {
      channel_->setPersistent(false);
      responseInfo.headers().push_back("Connection", "closed");
    }
  }

  generator_.generateResponse(responseInfo, chunk, false, &writer_);
  onComplete_ = onComplete;

  const bool corking_ = true;  // TODO(TCP_CORK): part of HttpResponseInfo?
  if (corking_)
    endpoint()->setCorking(true);

  wantFlush();
}

void HttpConnection::send(const BufferRef& chunk,
                          CompletionHandler&& onComplete) {
  if (onComplete && onComplete_)
    throw std::runtime_error("there is still another completion hook.");

  TRACE("%p send(chunkSize=%zu)", this, chunk.size());

  generator_.generateBody(chunk, false, &writer_);
  onComplete_ = std::move(onComplete);

  wantFlush();
}

void HttpConnection::send(FileRef&& chunk, CompletionHandler&& onComplete) {
  if (onComplete && onComplete_)
    throw std::runtime_error("there is still another completion hook.");

  TRACE("%p send(chunkSize=%zu)", this, chunk.size());

  generator_.generateBody(std::forward<FileRef>(chunk), false, &writer_);
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

  size_t n = parser_.parseFragment(inputBuffer_.ref(inputOffset_));
  inputOffset_ += n;
}

void HttpConnection::onFlushable() {
  TRACE("%p onFlushable", this);

  const bool complete = writer_.flush(endpoint());
  if (complete) {
    wantFill(); // restore interest to NONE or READ

    if (onComplete_) {
      auto callback = std::move(onComplete_);
      callback(true);
    }
  }
}

void HttpConnection::onInterestFailure(const std::exception& error) {
  if (!channel_->response()->isCommitted()) {
    // channel_->response()->sendError(HttpStatus::InternalServerError,
    //                                 error.what());

    channel_->setPersistent(false);
    channel_->response()->setStatus(HttpStatus::InternalServerError);
    channel_->response()->setReason(error.what());
    channel_->response()->headers().reset();
    channel_->response()->addHeader("Cache-Control",
                                    "no-cache,no-store,must-revalidate");
    channel_->response()->completed();

    // TODO: add some nice body and some special heades, such as cache-control
  } else {
    abort();
  }
}

}  // namespace http1
}  // namespace xzero
