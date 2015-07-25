// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-http/fastcgi/Connection.h>
#include <cortex-http/HttpChannel.h>
#include <cortex-http/HttpBufferedInput.h>
#include <cortex-http/HttpDateGenerator.h>
#include <cortex-http/HttpResponseInfo.h>
#include <cortex-http/HttpResponse.h>
#include <cortex-http/HttpRequest.h>
#include <cortex-http/BadMessage.h>
#include <cortex-base/net/Connection.h>
#include <cortex-base/net/EndPoint.h>
#include <cortex-base/net/EndPointWriter.h>
#include <cortex-base/executor/Executor.h>
#include <cortex-base/logging.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/WallClock.h>
#include <cortex-base/sysconfig.h>
#include <cassert>
#include <cstdlib>

namespace cortex {
namespace http {
namespace fastcgi {

/* TODO
 *
 * - how to handle BadMessage exceptions
 * - test this class for multiplexed requests
 * - test this class for multiplexed responses
 * - test this class for early client aborts
 * - test this class for early server aborts
 */

#ifndef NDEBUG
#define TRACE_CONN(msg...) logTrace("http.fastcgi.Connection", msg)
#define TRACE_TRANSPORT(msg...) logTrace("http.fastcgi.Transport", msg)
#define TRACE_CHANNEL(msg...) logTrace("http.fastcgi.Channel", msg)
#else
#define TRACE_CONN(msg...) do {} while (0)
#define TRACE_TRANSPORT(msg...) do {} while (0)
#define TRACE_CHANNEL(msg...) do {} while (0)
#endif

class HttpFastCgiTransport : public HttpTransport { // {{{
 public:
  HttpFastCgiTransport(Connection* connection,
                       int id, EndPointWriter* writer);
  virtual ~HttpFastCgiTransport();

  void setChannel(HttpChannel* channel) { channel_ = channel; }

  void abort() override;
  void completed() override;
  void send(HttpResponseInfo&& responseInfo, const BufferRef& chunk, CompletionHandler onComplete) override;
  void send(HttpResponseInfo&& responseInfo, Buffer&& chunk, CompletionHandler onComplete) override;
  void send(HttpResponseInfo&& responseInfo, FileRef&& chunk, CompletionHandler onComplete) override;
  void send(Buffer&& chunk, CompletionHandler onComplete) override;
  void send(FileRef&& chunk, CompletionHandler onComplete) override;
  void send(const BufferRef& chunk, CompletionHandler onComplete) override;

 private:
  void setCompleter(CompletionHandler ch);
  void onResponseComplete(bool success);

 private:
  Connection* connection_;
  HttpChannel* channel_;
  int id_;
  Generator generator_;
  CompletionHandler onComplete_;
};

void HttpFastCgiTransport::setCompleter(CompletionHandler onComplete) {
  if (!onComplete)
    return;

  if (onComplete_) {
    // "there is still another completion hook."
    RAISE(IllegalStateError);
  }

  onComplete_ = onComplete;

  connection_->onComplete_.emplace_back([this](bool success) {
    TRACE_TRANSPORT("%p setCompleter: callback(%s)", this, success ? "success" : "failed");
    auto cb = std::move(onComplete_);
    onComplete_ = nullptr;
    cb(success);
  });

  generator_.flushBuffer();
  connection_->wantFlush();
}

HttpFastCgiTransport::HttpFastCgiTransport(Connection* connection,
                                           int id,
                                           EndPointWriter* writer)
    : connection_(connection),
      channel_(nullptr),
      id_(id),
      generator_(id, writer) {
  TRACE_TRANSPORT("%p ctor", this);
}

HttpFastCgiTransport::~HttpFastCgiTransport() {
  TRACE_TRANSPORT("%p dtor", this);
}

void HttpFastCgiTransport::abort() { // TODO
  TRACE_TRANSPORT("%p abort!", this);
  RAISE(NotImplementedError);

  // channel_->response()->setBytesTransmitted(generator_.bytesTransmitted());
  // channel_->responseEnd();
  // endpoint()->close();
}

void HttpFastCgiTransport::completed() {
  TRACE_TRANSPORT("%p completed()", this);

  if (onComplete_)
    RAISE(IllegalStateError, "there is still another completion hook.");

  generator_.generateEnd();

  setCompleter(std::bind(&HttpFastCgiTransport::onResponseComplete, this,
                         std::placeholders::_1));
}

void HttpFastCgiTransport::onResponseComplete(bool success) {
  TRACE_TRANSPORT("%p onResponseComplete(%s)", this, success ? "success" : "failure");

  channel_->response()->setBytesTransmitted(generator_.bytesTransmitted());
  channel_->responseEnd();

  connection_->removeChannel(id_);
}

void HttpFastCgiTransport::send(HttpResponseInfo&& responseInfo, const BufferRef& chunk, CompletionHandler onComplete) {
  generator_.generateResponse(responseInfo, chunk);
  setCompleter(onComplete);
}

void HttpFastCgiTransport::send(HttpResponseInfo&& responseInfo, Buffer&& chunk, CompletionHandler onComplete) {
  generator_.generateResponse(responseInfo, std::move(chunk));
  setCompleter(onComplete);
}

void HttpFastCgiTransport::send(HttpResponseInfo&& responseInfo, FileRef&& chunk, CompletionHandler onComplete) {
  generator_.generateResponse(responseInfo, std::move(chunk));
  setCompleter(onComplete);
}

void HttpFastCgiTransport::send(Buffer&& chunk, CompletionHandler onComplete) {
  generator_.generateBody(std::move(chunk));
  setCompleter(onComplete);
}

void HttpFastCgiTransport::send(FileRef&& chunk, CompletionHandler onComplete) {
  generator_.generateBody(std::move(chunk));
  setCompleter(onComplete);
}

void HttpFastCgiTransport::send(const BufferRef& chunk, CompletionHandler onComplete) {
  generator_.generateBody(chunk);
  setCompleter(onComplete);
}
// }}}
class HttpFastCgiChannel : public HttpChannel { // {{{
 public:
  HttpFastCgiChannel(HttpTransport* transport,
                     const HttpHandler& handler,
                     std::unique_ptr<HttpInput>&& input,
                     size_t maxRequestUriLength,
                     size_t maxRequestBodyLength,
                     HttpDateGenerator* dateGenerator,
                     HttpOutputCompressor* outputCompressor);
  ~HttpFastCgiChannel();
};

HttpFastCgiChannel::HttpFastCgiChannel(
    HttpTransport* transport,
    const HttpHandler& handler,
    std::unique_ptr<HttpInput>&& input,
    size_t maxRequestUriLength,
    size_t maxRequestBodyLength,
    HttpDateGenerator* dateGenerator,
    HttpOutputCompressor* outputCompressor)
    : HttpChannel(transport,
                  handler,
                  std::move(input),
                  maxRequestUriLength,
                  maxRequestBodyLength,
                  dateGenerator,
                  outputCompressor) {
}

HttpFastCgiChannel::~HttpFastCgiChannel() {
  TRACE_CHANNEL("%p dtor", this);
  // we own the transport
  delete transport_;
}
// }}}

Connection::Connection(EndPoint* endpoint,
                       Executor* executor,
                       const HttpHandler& handler,
                       HttpDateGenerator* dateGenerator,
                       HttpOutputCompressor* outputCompressor,
                       size_t maxRequestUriLength,
                       size_t maxRequestBodyLength,
                       TimeSpan maxKeepAlive)
    : ::cortex::Connection(endpoint, executor),
      handler_(handler),
      maxRequestUriLength_(maxRequestUriLength),
      maxRequestBodyLength_(maxRequestBodyLength),
      dateGenerator_(dateGenerator),
      outputCompressor_(outputCompressor),
      maxKeepAlive_(maxKeepAlive),
      inputBuffer_(),
      inputOffset_(0),
      persistent_(false),
      parser_(
          std::bind(&Connection::onCreateChannel, this, std::placeholders::_1, std::placeholders::_2),
          std::bind(&Connection::onUnknownPacket, this, std::placeholders::_1, std::placeholders::_2),
          std::bind(&Connection::onAbortRequest, this, std::placeholders::_1)),
      channels_(),
      writer_(),
      onComplete_() {

  TRACE_CONN("%p ctor", this);
}

Connection::~Connection() {
  TRACE_CONN("%p dtor", this);
}

void Connection::onOpen() {
  TRACE_CONN("%p onOpen", this);
  cortex::Connection::onOpen();

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

void Connection::onClose() {
  TRACE_CONN("%p onClose", this);
  cortex::Connection::onClose();
}

void Connection::setInputBufferSize(size_t size) {
  TRACE_CONN("%p setInputBufferSize(%zu)", this, size);
  inputBuffer_.reserve(size);
}

void Connection::onFillable() {
  TRACE_CONN("%p onFillable", this);

  TRACE_CONN("%p onFillable: calling fill()", this);
  if (endpoint()->fill(&inputBuffer_) == 0) {
    TRACE_CONN("%p onFillable: fill() returned 0", this);
    // RAISE("client EOF");
    endpoint()->close();
    return;
  }

  parseFragment();
}

void Connection::parseFragment() {
  TRACE_CONN("parseFragment: calling parseFragment (%zu into %zu)",
             inputOffset_, inputBuffer_.size());
  size_t n = parser_.parseFragment(inputBuffer_.ref(inputOffset_));
  TRACE_CONN("parseFragment: called (%zu into %zu) => %zu",
             inputOffset_, inputBuffer_.size(), n);
  inputOffset_ += n;
}

void Connection::onFlushable() {
  TRACE_CONN("%p onFlushable", this);

  const bool complete = writer_.flush(endpoint());

  if (complete) {
    TRACE_CONN("%p onFlushable: completed. (%s)",
          this,
          (!onComplete_.empty() ? "onComplete cb set" : "onComplete cb not set"));

    if (!onComplete_.empty()) {
      TRACE_CONN("%p onFlushable: invoking completion %zu callback(s)", this, onComplete_.size());
      auto callbacks = std::move(onComplete_);
      onComplete_.clear();
      for (const auto& hook: callbacks) {
        TRACE_CONN("%p onFlushable: invoking one cb", this);
        hook(true);
      }
    }
  } else {
    // continue flushing as we still have data pending
    wantFlush();
  }
}

void Connection::onInterestFailure(const std::exception& error) {
  TRACE_CONN("%p onInterestFailure(%s): %s", this, typeid(error).name(), error.what());

  // TODO: improve logging here, as this eats our exception here.
  // e.g. via (factory or connector)->error(error);
  logError("fastcgi", error);

  auto callback = std::move(onComplete_);
  onComplete_.clear();

  // notify the callback that we failed doing something wrt. I/O.
  if (!callback.empty()) {
    TRACE_CONN("%p onInterestFailure: invoking onComplete(false)", this);
    for (const auto& hook: onComplete_) {
      hook(false);
    }
  }

  endpoint()->close();
}

HttpListener* Connection::onCreateChannel(int request, bool keepAlive) {
  TRACE_CONN("%p onCreateChannel(requestID=%d, keepalive=%s)",
             this, request, keepAlive ? "yes" : "no");
  setPersistent(keepAlive);
  return createChannel(request);
}

void Connection::onUnknownPacket(int request, int record) {
  TRACE_CONN("%p onUnknownPacket: request=%d, record=%d %s",
        this, request, record, to_string(static_cast<Type>(record)).c_str());
}

void Connection::onAbortRequest(int request) {
  removeChannel(request);
}

HttpChannel* Connection::createChannel(int request) {
  if (channels_.find(request) != channels_.end()) {
    RAISE(IllegalStateError,
          "FastCGI channel with ID %i already present.",
          request);
  }

  try {
    auto transport = new HttpFastCgiTransport(this, request, &writer_);
    std::unique_ptr<HttpFastCgiChannel> channel(new HttpFastCgiChannel(
       transport,
       handler_,
       std::unique_ptr<HttpInput>(new HttpBufferedInput()),
       maxRequestUriLength_,
       maxRequestBodyLength_,
       dateGenerator_,
       outputCompressor_));

    transport->setChannel(channel.get());
    channel->request()->setRemoteIP(endpoint()->remoteIP());

    return (channels_[request] = std::move(channel)).get();
  } catch (...) {
    persistent_ = false;
    removeChannel(request);
    throw;
  }
}

void Connection::removeChannel(int request) {
  TRACE_CONN("%p removeChannel(%d) %s",
             this, request, isPersistent() ? "keepalive" : "close");

  auto i = channels_.find(request);
  if (i != channels_.end()) {
    channels_.erase(i);
  }

  parser_.removeStreamState(request);

  if (isPersistent()) {
    wantFill();
  } else if (channels_.empty()) {
    endpoint()->close();
  }
}

void Connection::setPersistent(bool enable) {
  TRACE_CONN("setPersistent(%s) (timeout=%ds)",
      enable ? "yes" : "no",
      maxKeepAlive_.totalSeconds());
  if (maxKeepAlive_ != TimeSpan::Zero) {
    persistent_ = enable;
  }
}

} // namespace fastcgi
} // namespace http
} // namespace cortex
