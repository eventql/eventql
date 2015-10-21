/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Laura Schlimmer
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/inspect.h>
#include <stx/exception.h>
#include <stx/http/HTTPResponseStream.h>

namespace stx {
namespace http {

HTTPResponseStream::HTTPResponseStream(
    HTTPServerConnection* conn) :
    conn_(conn),
    callback_running_(false),
    headers_written_(false),
    response_finished_(false) {}

void HTTPResponseStream::writeResponse(HTTPResponse res) {
  auto body_size = res.body().size();
  if (body_size > 0) {
    res.setHeader("Content-Length", StringUtil::toString(body_size));
  }

  startResponse(res);
  finishResponse();
}

void HTTPResponseStream::startResponse(const HTTPResponse& resp) {
  std::unique_lock<std::mutex> lk(mutex_);

  if (headers_written_) {
    RAISE(kRuntimeError, "headers already written");
  }

  headers_written_ = true;
  callback_running_ = true;
  lk.unlock();

  conn_->writeResponse(
    resp,
    std::bind(&HTTPResponseStream::onCallbackCompleted, this));
}

void HTTPResponseStream::writeBodyChunk(const VFSFile& buf) {
  writeBodyChunk(buf.data(), buf.size());
}

void HTTPResponseStream::writeBodyChunk(const void* data, size_t size) {
  std::unique_lock<std::mutex> lk(mutex_);
  buf_.append(data, size);
  onStateChanged(&lk);
}

void HTTPResponseStream::finishResponse() {
  std::unique_lock<std::mutex> lk(mutex_);

  if (response_finished_) {
    return;
  }

  response_finished_ = true;
  onStateChanged(&lk);
}

bool HTTPResponseStream::isOutputStarted() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return headers_written_;
}

bool HTTPResponseStream::isClosed() const {
  return conn_->isClosed();
}

void HTTPResponseStream::onCallbackCompleted() {
  std::unique_lock<std::mutex> lk(mutex_);
  callback_running_ = false;
  onStateChanged(&lk);
}

void HTTPResponseStream::onBodyWritten(Function<void ()> callback) {
  std::unique_lock<std::mutex> lk(mutex_);
  on_body_written_ = callback;
}

size_t HTTPResponseStream::bufferSize() {
  std::unique_lock<std::mutex> lk(mutex_);
  return buf_.size();
}

void HTTPResponseStream::waitForReader() {
  std::unique_lock<std::mutex> lk(mutex_);

  while (buf_.size() > kMaxWriteBufferSize) {
    cv_.wait(lk);
  }
}

// precondition: lk must be locked
void HTTPResponseStream::onStateChanged(std::unique_lock<std::mutex>* lk) {
  if (callback_running_) {
    return;
  }

  if (!headers_written_) {
    return; // should never happen!
  }

  if (buf_.size() > 0) {
    Buffer write_buf = buf_;
    buf_.clear();

    if (write_buf.size() > kMaxWriteBufferSize) {
      buf_.append(
          (char*) write_buf.data() + kMaxWriteBufferSize,
          write_buf.size() - kMaxWriteBufferSize);

      write_buf.truncate(kMaxWriteBufferSize);
    }

    cv_.notify_all();
    callback_running_ = true;
    lk->unlock();

    conn_->writeResponseBody(
        write_buf.data(),
        write_buf.size(),
        std::bind(&HTTPResponseStream::onCallbackCompleted, this));

    return;
  }

  if (response_finished_) {
    conn_->finishResponse();
    lk->unlock();

    decRef();
  } else {
    lk->unlock();

    if (on_body_written_) {
      on_body_written_();
    }
  }
}

}
}

