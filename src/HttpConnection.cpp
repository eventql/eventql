// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/HttpConnection.h>
#include <xzero/HttpRequest.h>
#include <xzero/HttpWorker.h>
#include <base/io/BufferRefSource.h>
#include <base/io/BufferSource.h>
#include <base/io/FileSource.h>
#include <base/ServerSocket.h>
#include <base/SocketDriver.h>
#include <base/StackTrace.h>
#include <base/Socket.h>
#include <base/Types.h>
#include <base/DebugLogger.h>
#include <base/sysconfig.h>

#include <functional>
#include <cstdarg>
#include <cstdlib>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>

#if !defined(NDEBUG)
#define TRACE(level, msg...) XZERO_DEBUG("HttpConnection", (level), msg)
#else
#define TRACE(msg...) \
  do {                \
  } while (0)
#endif

namespace xzero {

/**
 * \class HttpConnection
 * \brief represents an HTTP connection handling incoming requests.
 *
 * The \p HttpConnection object is to be allocated once an HTTP client connects
 * to the HTTP server and was accepted by the \p ServerSocket.
 * It will also own the respective request and response objects created to serve
 * the requests passed through this connection.
 */

/* TODO
 * - should request bodies land in requestBuffer_? someone with a good line
 * could flood us w/ streaming request bodies.
 */

std::string HttpConnection::tempDirectory_("/tmp");

const std::string& HttpConnection::tempDirectory() {
  return tempDirectory_;
}

void HttpConnection::setTempDirectory(const char* path) {
  if (path && *path) {
    tempDirectory_ = path;
  }
}

/**
 * @brief initializes a new connection object.
 *
 * @note This triggers the onConnectionOpen event.
 */
HttpConnection::HttpConnection(HttpWorker* w, unsigned long long id)
    : HttpMessageParser(HttpMessageParser::REQUEST),
      refCount_(0),
      state_(Undefined),
      worker_(w),
      id_(id),
      requestCount_(0),
      shouldKeepAlive_(false),
      clientAbortHandler_(),
      requestBuffer_(worker().server().requestHeaderBufferSize() +
                     worker().server().requestBodyBufferSize()),
      requestParserOffset_(0),
      request_(nullptr),
      requestBodyPath_(),
      requestBodyFd_(-1),
      requestBodyFileSize_(0),
      output_(),
      socket_(nullptr),
      sink_(nullptr),
      autoFlush_(true),
      prev_(nullptr),
      next_(nullptr) {
  requestBodyPath_[0] = '\0';
}

/** releases all connection resources  and triggers the onConnectionClose event.
 */
HttpConnection::~HttpConnection() {
  TRACE(1, "%d: destructing", id_);
  if (request_) {
    delete request_;
  }
  clearRequestBody();
}

void HttpConnection::reinit(unsigned long long id) {
  id_ = id;
  shouldKeepAlive_ = false;
  requestCount_ = 0;

  requestBuffer_.clear();
  requestParserOffset_ = 0;

  HttpMessageParser::reset();
}

void HttpConnection::clearRequestBody() {
  if (requestBodyFd_ >= 0) {
    ::close(requestBodyFd_);
    requestBodyFd_ = -1;
  }

  if (requestBodyPath_[0] != '\0') {
    unlink(requestBodyPath_);
    requestBodyPath_[0] = '\0';
  }
}

void HttpConnection::ref() {
  ++refCount_;
  TRACE(1, "ref() %u", refCount_);
}

void HttpConnection::unref() {
  --refCount_;

  TRACE(1, "unref() %u (conn:%s, outputPending:%d, cstate:%s, pstate:%s)",
        refCount_, isOpen() ? "open" : "closed", isOutputPending(), state_str(),
        tos(parserState()).c_str());
  // TRACE(1, "Stack Trace:\n%s", StackTrace().c_str());

  if (refCount_ > 0) return;

  // XXX connection is to be closed and its resources potentially reused later.

  if (request_) request_->clear();

  clearCustomData();

  worker_->server_.onConnectionClose(this);
  socket_.reset(nullptr);

  requestParserOffset_ = 0;
  requestBuffer_.clear();

  worker_->release(this);
}

void HttpConnection::onReadWriteReady(Socket*, int revents) {
  TRACE(1, "io(revents=%04x) %s, %s", revents, state_str(), tos(parserState()).c_str());

  ScopedRef _r(this);

  if (revents & ev::ERROR) {
    log(Severity::error, "Potential bug in connection I/O watching. Closing.");
    abort();
    return;
  }

  // socket is ready for read?
  if (revents & Socket::Read) {
    if (!readSome()) {
      return;
    }
  }

  // socket is ready for write?
  if (revents & Socket::Write) {
    if (!writeSome()) {
      return;
    }
  }
}

void HttpConnection::onReadWriteTimeout(Socket*) {
  TRACE(1, "timeout(): cstate:%s, pstate:%s", state_str(), tos(parserState()).c_str());

  switch (state()) {
    case ReadingRequest:
    case KeepAliveRead:
      abort(HttpStatus::RequestTimeout);
      break;
    case ProcessingRequest:
    case SendingReply:
    case SendingReplyDone:
    case Undefined:
      abort();
      break;
  }
}

bool HttpConnection::isSecure() const {
#if defined(WITH_SSL)
  return socket_->isSecure();
#else
  return false;
#endif
}

/**
 * Start first non-blocking operation for this HttpConnection.
 *
 * This is done by simply registering the underlying socket to the the I/O
 *service
 * to watch for available input.
 *
 * @note This method must be invoked right after the object construction.
 *
 * @see close()
 * @see abort()
 * @see abort(HttpStatus)
 */
void HttpConnection::start(std::unique_ptr<Socket>&& client,
                           bool acceptDeferred) {
  setState(ReadingRequest);

  socket_ = std::move(client);
  socket_->setReadyCallback<HttpConnection, &HttpConnection::onReadWriteReady>(
      this);

  sink_.setSocket(socket_.get());

#if defined(TCP_NODELAY)
  if (worker_->server().tcpNoDelay()) socket_->setTcpNoDelay(true);
#endif

  if (worker_->server().lingering())
    socket_->setLingering(worker_->server().lingering());

  TRACE(1, "starting (fd=%d)", socket_->handle());

  ref();  // <-- this reference is being decremented in close()

  ScopedRef _r(this);

  worker_->server_.onConnectionOpen(this);

  if (!isOpen()) {
    // The connection got directly closed within the onConnectionOpen-callback,
    // so delete the object right away.
    return;
  }

  if (!request_) request_ = new HttpRequest(*this);

  if (socket_->state() == Socket::Handshake) {
    TRACE(1, "start: handshake.");
    socket_->handshake<HttpConnection, &HttpConnection::onHandshakeComplete>(
        this);
  } else {
    if (acceptDeferred) {
      TRACE(1, "start: processing input");

      // initialize I/O watching state to READ
      wantRead(worker_->server_.maxReadIdle());

      // TCP_DEFER_ACCEPT attempts to let us accept() only when data has arrived.
      // Thus we can go straight and attempt to read it.
      readSome();

      TRACE(1, "start: processing input done (state: %s)", state_str());
    } else {
      TRACE(1, "start: wantRead.");
      // client connected, but we do not yet know if we have data pending
      wantRead(worker_->server_.maxReadIdle());
    }
  }
}

void HttpConnection::onHandshakeComplete(Socket*) {
  TRACE(1, "onHandshakeComplete() socketState=%s", socket_->state_str());

  if (socket_->state() == Socket::Operational) {
    wantRead(worker_->server_.maxReadIdle());
  } else {
    TRACE(1, "onHandshakeComplete(): handshake failed\n%s",
          StackTrace().c_str());
    abort();
  }
}

bool HttpConnection::onMessageBegin(const BufferRef& method,
                                    const BufferRef& uri, int versionMajor,
                                    int versionMinor) {
  TRACE(1, "onMessageBegin: '%s', '%s', HTTP/%d.%d", method.str().c_str(),
        uri.str().c_str(), versionMajor, versionMinor);

  request_->method = method;
  request_->timeStart_ = worker().now();

  if (!request_->setUri(uri)) {
    abort(HttpStatus::BadRequest);
    return false;
  }

  request_->httpVersionMajor = versionMajor;
  request_->httpVersionMinor = versionMinor;

  // HTTP/1.1 is keeping alive by default. pass "Connection: close" to close
  // explicitely
  setShouldKeepAlive(request_->supportsProtocol(1, 1));

  // limit request URI length
  if (request_->unparsedUri.size() > worker().server().maxRequestUriSize()) {
    request_->status = HttpStatus::RequestUriTooLong;
    request_->finish();
    return false;
  }

  return true;
}

bool HttpConnection::onMessageHeader(const BufferRef& name,
                                     const BufferRef& value) {
  if (request_->isFinished()) {
    // this can happen when the request has failed some checks and thus,
    // a client error message has been sent already.
    // we need to "parse" the remaining content anyways.
    TRACE(1, "onMessageHeader() skip \"%s\": \"%s\"", name.str().c_str(),
          value.str().c_str());
    return true;
  }

  TRACE(1, "onMessageHeader() \"%s\": \"%s\"", name.str().c_str(),
        value.str().c_str());

  if (iequals(name, "Host")) {
    auto i = value.find(':');
    if (i != BufferRef::npos)
      request_->hostname = value.ref(0, i);
    else
      request_->hostname = value;
    TRACE(1, " -- hostname set to \"%s\"", request_->hostname.str().c_str());
  } else if (iequals(name, "Connection")) {
    if (iequals(value, "close"))
      setShouldKeepAlive(false);
    else if (iequals(value, "keep-alive"))
      setShouldKeepAlive(true);
  }

  // limit the size of a single request header
  if (name.size() + value.size() > worker().server().maxRequestHeaderSize()) {
    TRACE(1, "header too long. got %lu / %lu", name.size() + value.size(),
          worker().server().maxRequestHeaderSize());
    abort(HttpStatus::RequestHeaderFieldsTooLarge);
    return false;
  }

  // limit the number of request headers
  if (request_->requestHeaders.size() >=
      worker().server().maxRequestHeaderCount()) {
    abort(HttpStatus::RequestHeaderFieldsTooLarge);
    return false;
  }

  request_->requestHeaders.push_back(HttpRequestHeader(name, value));
  return true;
}

bool HttpConnection::onMessageHeaderEnd() {
  TRACE(1, "onMessageHeaderEnd() requestParserOffset:%zu",
        requestParserOffset_);

  if (request_->isFinished())  // FIXME should never happen (anymore?)
    return true;

  ++requestCount_;
  requestHeaderEndOffset_ = requestParserOffset_;

  const bool contentRequired =
      request_->method == "POST" || request_->method == "PUT";

  if (contentRequired) {
    if (request_->connection.contentLength() == -1 &&
        !request_->connection.isChunked()) {
      abort(HttpStatus::LengthRequired);
      return false;
    }
    if (static_cast<size_t>(request_->connection.contentLength()) >
        worker().server().maxRequestBodySize()) {
      request_->expectingContinue = false;  // do not submit a '100-continue'
      abort(HttpStatus::PayloadTooLarge);
      return false;
    }
  } else {
    if (request_->contentAvailable()) {
      abort(HttpStatus::BadRequest);  // FIXME do we have a better status code?
      return false;
    }
  }

  const BufferRef expectHeader = request_->requestHeader("Expect");

  if (expectHeader) {
    request_->expectingContinue = equals(expectHeader, "100-continue");

    if (!request_->expectingContinue || !request_->supportsProtocol(1, 1)) {
      abort(HttpStatus::ExpectationFailed);
      return false;
    }
  }

  if (isContentExpected()) {
    if (request_->expectingContinue) {
      TRACE(1, "onMessageHeaderEnd() sending 100-continue");
      write<BufferRefSource>("HTTP/1.1 100 Continue\r\n\r\n");
      flush();
      // FIXME: call request handler only after this one has been sent out
      // (successfully).
      request_->expectingContinue = false;
    }

    const bool useTmpFile =
        isChunked() ||
        contentLength() >
            static_cast<ssize_t>(worker().server().requestBodyBufferSize());

    if (!useTmpFile) {
      request_->body_.reset(new BufferSource());
    } else {
      const int flags = O_RDWR;

#if defined(O_TMPFILE)
      static bool otmpfileSupported = true;
      if (otmpfileSupported) {
        requestBodyFd_ = open(tempDirectory_.c_str(), flags | O_TMPFILE);
        if (requestBodyFd_ >= 0) {
          // explicitely mark it as O_TMPFILE-opened
          requestBodyPath_[0] = '\0';
        } else {
          // do not attempt to try it again
          otmpfileSupported = false;
        }
      }
#endif
      if (requestBodyFd_ < 0) {
        snprintf(requestBodyPath_, sizeof(requestBodyPath_),
                 "%s/x0d-request-body-XXXXXX", tempDirectory_.c_str());
        requestBodyFd_ = mkostemp(requestBodyPath_, flags);
      }
    }
  }

  return true;
}

bool HttpConnection::onMessageContent(const BufferRef& chunk) {
  TRACE(1, "onMessageContent(#%lu): cstate:%s pstate:%s", chunk.size(),
        state_str(), tos(parserState()).c_str());

  if (requestBodyFd_ >= 0) {
    TRACE(1, "onMessageContent: write to fd %d (%zu bytes)", requestBodyFd_,
          chunk.size());
    ssize_t rv = ::write(requestBodyFd_, chunk.data(), chunk.size());
    if (rv < 0) {
      if (requestBodyPath_[0] != '\0') {
        log(Severity::error,
            "Failed to write %zu bytes to temporary request body file (%s). %s",
            chunk.size(), requestBodyPath_, strerror(errno));
      } else {
        log(Severity::error,
            "Failed to write %zu bytes to temporary request body file. %s",
            chunk.size(), strerror(errno));
      }
      abort(HttpStatus::InternalServerError);
      return false;
    }
    requestBodyFileSize_ += rv;
  } else if (request_->body_.get()) {
    TRACE(1, "onMessageContent: write buf chunk (%zu bytes)", chunk.size());
    static_cast<BufferSource*>(request_->body_.get())->buffer().push_back(
        chunk);
    return true;
  } else {
    TRACE(1, "onMessageContent: ignore chunk (%zu bytes)", chunk.size());
    // probably something blased up on the host, so don't push it
    // and ignore the rquest body. the app has to deal with this case,
    // and the admin should be aware of the log entries.
  }

  return true;
}

bool HttpConnection::onMessageEnd() {
  TRACE(1, "onMessageEnd() %s (rfinished:%d)", state_str(),
        request_->isFinished());

  if (requestBodyFd_ >= 0) {
    request_->body_.reset(
        new FileSource(requestBodyFd_, 0, requestBodyFileSize_, true));
    requestBodyFd_ = -1;
  }

  setState(ProcessingRequest);
  worker_->handleRequest(request_);

  // We are currently procesing a request, so stop parsing at the end of this
  // request.
  // The next request, if available, is being processed via resume()
  return false;
}

static inline Buffer escapeChunk(const BufferRef& chunk) {
  Buffer result;

  for (char ch : chunk) {
    switch (ch) {
      case '\r':
        result.push_back("\\r");
        break;
      case '\n':
        result.push_back("\\n");
        break;
      case '\t':
        result.push_back("\\t");
        break;
      case '"':
        result.push_back("\\\"");
        break;
      default:
        if (std::isprint(ch) || std::isspace(ch)) {
          result.push_back(ch);
        } else {
          result.printf("\\x%02x", ch);
        }
    }
  }

  return result;
}

void HttpConnection::onProtocolError(const BufferRef& chunk, size_t offset) {
  log(Severity::debug, "HTTP protocol error at chunk offset %zu (0x%02x): %s",
      offset, chunk[offset], tos(parserState()).c_str());

  log(Severity::trace, "Request parser offset: %zu", requestParserOffset_);
  log(Severity::trace, "Request Buffer: \"%s\"",
      escapeChunk(requestBuffer_.ref(0, std::min(requestParserOffset_ + 1,
                                                 requestBuffer_.size())))
          .c_str());
}

void HttpConnection::wantRead(const TimeSpan& timeout) {
  TRACE(3, "wantRead(): cstate:%s pstate:%s", state_str(),
        tos(parserState()).c_str());

  if (timeout)
    socket_->setTimeout<HttpConnection, &HttpConnection::onReadWriteTimeout>(
        this, timeout.value());

  socket_->setMode(Socket::Read);
}

void HttpConnection::wantWrite() {
  TRACE(3, "wantWrite(): cstate:%s pstate:%s", state_str(), tos(parserState()).c_str());

  if (isContentExpected()) {
    auto timeout = std::max(worker().server().maxReadIdle().value(),
                            worker().server().maxWriteIdle().value());

    socket_->setTimeout<HttpConnection, &HttpConnection::onReadWriteTimeout>(
        this, timeout);
    socket_->setMode(Socket::ReadWrite);
  } else {
    auto timeout = worker().server().maxWriteIdle().value();
    socket_->setTimeout<HttpConnection, &HttpConnection::onReadWriteTimeout>(
        this, timeout);

    socket_->setMode(Socket::Write);
  }
}

/**
 * This method gets invoked when there is data in our HttpConnection ready to
 *read.
 *
 * We assume, that we are in request-parsing state.
 */
bool HttpConnection::readSome() {
  TRACE(1,
        "readSome: cstate=%s pstate=%s, parserOffset=%zu, "
        "requestBuffer.size=%zu/%zu",
        state_str(), tos(parserState()).c_str(), requestParserOffset_,
        requestBuffer_.size(), requestBuffer_.capacity());

  if (requestBuffer_.size() == requestBuffer_.capacity()) {
    TRACE(1,
          "readSome: reached request buffer limit, not reading from client.");
    return true;
  }

  if (requestParserOffset_ == requestBuffer_.size()) {
    ssize_t rv = socket_->read(requestBuffer_, requestBuffer_.capacity());
    TRACE(1, "readSome: read %li bytes", rv);

    if (rv < 0) {  // error
      switch (errno) {
        case EINTR:
        case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
        case EWOULDBLOCK:
#endif
          wantRead(worker_->server_.maxReadIdle());
          return true;
        default:
          if (state() != KeepAliveRead) {
            log(Severity::error, "(%s) Failed to read from client. %s",
                tos(parserState()).c_str(), strerror(errno));
            request_->status = HttpStatus::Hangup;
          }
          abort();
          return false;
      }
    } else if (rv == 0) {  // EOF
      TRACE(1, "readSome: (EOF), state:%s, clientAbortHandler:%s", state_str(),
            clientAbortHandler_ ? "yes" : "no");

      request_->status = HttpStatus::Hangup;
      if (clientAbortHandler_) {
        socket_->close();
        clientAbortHandler_();
      } else {
        abort();
      }

      return false;
    }
  }

  if (state() == KeepAliveRead) {
    TRACE(1,
          "readSome: state was keep-alive-read. resetting to reading-request");
    setState(ReadingRequest);
  }

  {
    // ScopedRef _r(this);

    process();

    if (isProcessingBody() && requestParserOffset_ > requestHeaderEndOffset_) {
      // adjusting buffer for next body-chunk reads
      TRACE(1, "readSome: rewind requestBuffer end from %zu to %zu",
            requestParserOffset_, requestHeaderEndOffset_);

      requestParserOffset_ = requestHeaderEndOffset_;
      requestBuffer_.resize(requestHeaderEndOffset_);
    }
  }

  return true;
}

/** write source into the connection stream and notifies the handler on
 *completion.
 *
 * \param buffer the buffer of bytes to be written into the connection.
 * \param handler the completion handler to invoke once the buffer has been
 *either fully written or an error occured.
 */
void HttpConnection::write(std::unique_ptr<Source>&& chunk) {
  if (isOpen()) {
    TRACE(1, "write() chunk (%s) autoFlush:%s", chunk->className(),
          autoFlush_ ? "yes" : "no");
    output_.push_back(std::move(chunk));

    if (autoFlush_) {
      flush();
    }
  } else {
    TRACE(1, "write() ignore chunk (%s) - (connection aborted)",
          chunk->className());
  }
}

void HttpConnection::flush() {
  TRACE(1, "flush() (isOutputPending:%d)", isOutputPending());

  if (!isOutputPending()) return;

#if defined(XZERO_OPPORTUNISTIC_WRITE)
  writeSome();
#else
  wantWrite();
#endif
}

/**
 * Writes as much as it wouldn't block of the response stream into the
 *underlying socket.
 *
 * @see wantWrite()
 * @see wantRead(), readSome()
 */
bool HttpConnection::writeSome() {
  TRACE(1, "writeSome() state: %s", state_str());

  for (;;) {
    ssize_t rv = output_.sendto(sink_);

    TRACE(1, "writeSome(): sendto().rv=%lu %s", rv,
          rv < 0 ? strerror(errno) : "");

    if (rv > 0) {
      // output chunk written
      request_->bytesTransmitted_ += rv;
      continue;
    }

    if (rv == 0) {
      // output fully written
      if (request_->isFinished()) {
        // finish() got invoked before reply was fully sent out, thus,
        // finalize() was delayed.
        request_->finalize();
      } else {
        // watch for EOF at least
        wantRead(TimeSpan::Zero);
      }

      TRACE(1,
            "writeSome: output fully written. conn:%s, outputPending:%lu, "
            "refCount:%d",
            isOpen() ? "open" : "closed", output_.size(), refCount_);

      return true;
    }

    // sendto() failed
    switch (errno) {
      case EINTR:
        break;
      case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
      case EWOULDBLOCK:
#endif
        wantWrite();
        return true;
      default:
        log(Severity::error, "Failed to write to client. %s", strerror(errno));
        request_->status = HttpStatus::Hangup;
        abort();
        return false;
    }
  }
}

/**
 * Aborts processing current request with given HTTP status code and closes the
 *connection.
 *
 * This method is supposed to be invoked within the Request parsing state.
 * It will reply the current request with the given \p status code and close the
 *connection.
 * This is kind of a soft abort, as the client will be notified about the soft
 *error with a proper reply.
 *
 * \param status the HTTP status code (should be a 4xx client error).
 *
 * \see abort()
 */
void HttpConnection::abort(HttpStatus status) {
  TRACE(1, "abort(%d): cstate:%s, pstate:%s", (int)status, state_str(),
        tos(parserState()).c_str());

  assert(state() == ReadingRequest || state() == KeepAliveRead);

  ++requestCount_;

  setState(ProcessingRequest);
  setShouldKeepAlive(false);

  request_->status = status;
  request_->finish();
}

/**
 * Internally invoked on I/O errors to prematurely close the connection without
 *any further processing.
 *
 * Invoked from within the following states:
 * <ul>
 *   <li> client read operation error
 *   <li> client write operation error
 *   <li> client read/write timeout
 *   <li> client closed connection
 * </ul>
 */
void HttpConnection::abort() {
  if (request_) {
    request_->onRequestDone();
    request_->connection.worker().server().onRequestDone(request_);
    request_->clearCustomData();
  }

  close();
}

/** Closes this HttpConnection, possibly deleting this object (or propagating
 * delayed delete).
 */
void HttpConnection::close() {
  TRACE(1, "close() (state:%s)", state_str());
  TRACE(2, "Stack Trace:%s\n", StackTrace().c_str());

  socket_->close();

  clientAbortHandler_ = std::function<void()>();

  if (isOutputPending()) {
    TRACE(1, "abort: clearing pending output (%lu)", output_.size());
    output_.clear();
  }

  setState(Undefined);

  unref();  // <-- this refers to ref() in start()
}

/** Resumes processing the <b>next</b> HTTP request message within this
 *connection.
 *
 * This method may only be invoked when the current/old request has been fully
 *processed,
 * and is though only be invoked from within the finish handler of \p
 *HttpRequest.
 *
 * \see HttpRequest::finish()
 */
void HttpConnection::resume() {
  TRACE(1, "resume() shouldKeepAlive:%d, cstate:%s, pstate:%s",
        shouldKeepAlive(), state_str(), tos(parserState()).c_str());
  TRACE(1, "-- (requestParserOffset:%lu, requestBufferSize:%lu)",
        requestParserOffset_, requestBuffer_.size());

  // move potential pipelined-and-already-read requests to the front and reset
  // parser offset
  requestBuffer_ = requestBuffer_.ref(
      requestParserOffset_, requestBuffer_.size() - requestParserOffset_);
  requestParserOffset_ = 0;
  TRACE(1, "-- moved %zu bytes pipelined request fragment data to the front",
        requestBuffer_.size());

  if (socket()->tcpCork()) socket()->setTcpCork(false);

  if (requestBuffer_.empty()) {
    setState(KeepAliveRead);
    wantRead(worker().server().maxKeepAlive());
  } else {
    // pipelined request
    setState(ReadingRequest);
    readSome();
  }
}

/** processes a (partial) request from buffer's given \p offset of \p count
 * bytes.
 */
bool HttpConnection::process() {
  TRACE(2, "process: offset=%lu, size=%lu (before processing) %s, %s",
        requestParserOffset_, requestBuffer_.size(), state_str(),
        tos(parserState()).c_str());

  while (parserState() != MESSAGE_BEGIN || state() == ReadingRequest ||
         state() == KeepAliveRead) {
    BufferRef chunk(requestBuffer_.ref(requestParserOffset_));
    if (chunk.empty()) break;

    // ensure state is up-to-date, in case we came from keep-alive-read
    if (state() == KeepAliveRead) {
      TRACE(1, "process: status=keep-alive-read, resetting to reading-request");
      setState(ReadingRequest);
      if (request_->isFinished()) {
        TRACE(1, "process: finalizing request");
        request_->finalize();
      }
    }

    TRACE(1, "process: (size: %lu, cstate:%s pstate:%s", chunk.size(),
          state_str(), tos(parserState()).c_str());
    // TRACE(1, "%s", requestBuffer_.ref(requestBuffer_.size() -
    // rv).str().c_str());

    size_t rv = HttpMessageParser::parseFragment(chunk, &requestParserOffset_);
    TRACE(1,
          "process: done process()ing; fd=%d, request=%p cstate:%s pstate:%s, "
          "rv:%d",
          socket_->handle(), request_, state_str(), tos(parserState()).c_str(),
          rv);

    if (!isOpen()) {
      TRACE(1, "abort detected");
      return false;
    }

    if (parserState() == PROTOCOL_ERROR) {
      if (!request_->isFinished()) {
        abort(HttpStatus::BadRequest);
      }
      return false;
    }

    if (isProcessingHeader() && !request_->isFinished()) {
      if (requestParserOffset_ >= worker().server().requestHeaderBufferSize()) {
        TRACE(1,
              "request too large -> 413 (requestParserOffset:%zu, "
              "requestBufferSize:%zu)",
              requestParserOffset_,
              worker().server().requestHeaderBufferSize());
        abort(HttpStatus::RequestHeaderFieldsTooLarge);
        return false;
      }
    }

    if (rv < chunk.size()) {
      request_->log(Severity::trace1, "parser aborted early.");
      return false;
    }
  }

  TRACE(1,
        "process: offset=%lu, bs=%lu, state=%s (after processing) io.timer:%d",
        requestParserOffset_, requestBuffer_.size(), state_str(),
        socket_->timerActive());

  return true;
}

void HttpConnection::setShouldKeepAlive(bool enabled) {
  TRACE(1, "setShouldKeepAlive: %d", enabled);

  shouldKeepAlive_ = enabled;
}

void HttpConnection::setState(State value) {
#if !defined(NDEBUG)
  static const char* str[] = {"undefined",          "reading-request",
                              "processing-request", "sending-reply",
                              "sending-reply-done", "keep-alive-read"};
  (void)str;
  TRACE(1, "setState() %s => %s", str[static_cast<size_t>(state_)],
        str[static_cast<size_t>(value)]);
#endif

  State lastState = state_;
  state_ = value;
  worker().server().onConnectionStateChanged(this, lastState);
}

void HttpConnection::log(LogMessage&& msg) {
  msg.addTag(isOpen() ? remoteIP().c_str() : "(null)");

  worker().log(std::forward<LogMessage>(msg));
}

void HttpConnection::post(std::function<void()>&& function) {
  ref();
  worker_->post([=]() {
    function();
    unref();
  });
}

}  // namespace xzero
