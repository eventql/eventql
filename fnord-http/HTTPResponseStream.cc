/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Laura Schlimmer
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/inspect.h>
#include <fnord-base/exception.h>
#include <fnord-http/HTTPResponseStream.h>

namespace fnord {
namespace http {

HTTPResponseStream::HTTPResponseStream(
    HTTPServerConnection* conn) :
    conn_(conn),
    callback_running_(false),
    response_finished_(false),
    headers_written_(false) {}

void HTTPResponseStream::startResponse(const HTTPResponse& resp) {
  std::unique_lock<std::mutex> lk(mutex_);

  if (headers_written_) {
    RAISE(kRuntimeError, "headers already written");
  }

  callback_running_ = true;
  lk.unlock();

  conn_->writeResponse(
    resp,
    std::bind(&HTTPResponseStream::onCallbackCompleted, this));
}

void HTTPResponseStream::writeBodyChunk(const Buffer& buf) {
  std::unique_lock<std::mutex> lk(mutex_);
  buf_.append(buf.data(), buf.size());
  onStateChanged();
}

void HTTPResponseStream::finishResponse() {
  std::unique_lock<std::mutex> lk(mutex_);

  if (response_finished_) {
    return;
  }

  response_finished_ = true;
  onStateChanged();
}

bool HTTPResponseStream::isOutputStarted() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return headers_written_;
}

void HTTPResponseStream::onCallbackCompleted() {
  std::unique_lock<std::mutex> lk(mutex_);
  callback_running_ = false;
  onStateChanged();
}

// precondition: hold mutex_
void HTTPResponseStream::onStateChanged() {
  if (callback_running_) {
    return;
  }

  if (response_finished_) {
    conn_->finishResponse();
    decRef();
  }
}

}
}

