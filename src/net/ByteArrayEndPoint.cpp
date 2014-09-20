#include <xzero/net/ByteArrayEndPoint.h>
#include <xzero/net/Connection.h>
#include <xzero/net/LocalConnector.h>
#include <xzero/executor/Executor.h>
#include <xzero/logging/LogSource.h>
#include <system_error>
#include <stdio.h>
#include <unistd.h>

namespace xzero {

static LogSource localEndPointLogger("net.ByteArrayEndPoint");
#ifndef NDEBUG
#define TRACE(msg...) do { localEndPointLogger.trace(msg); } while (0)
#else
#define TRACE(msg...) do {} while (0)
#endif

ByteArrayEndPoint::ByteArrayEndPoint(LocalConnector* connector)
    : connector_(connector),
      isBusy_(false),
      input_(),
      readPos_(0),
      output_(),
      closed_(false) {
}

ByteArrayEndPoint::~ByteArrayEndPoint() {
}

void ByteArrayEndPoint::setInput(Buffer&& input) {
  readPos_ = 0;
  input_ = std::move(input);
}

void ByteArrayEndPoint::setInput(const char* input) {
  readPos_ = 0;
  input_.clear();
  input_.push_back(input);
}

const Buffer& ByteArrayEndPoint::input() const {
  return input_;
}

const Buffer& ByteArrayEndPoint::output() const {
  return output_;
}

void ByteArrayEndPoint::close() {
  //closed_ = true;
}

bool ByteArrayEndPoint::isOpen() const {
  return !closed_;
}

std::string ByteArrayEndPoint::toString() const {
  char buf[256];
  snprintf(buf, sizeof(buf),
           "ByteArrayEndPoint@%p(readPos:%zu, writePos:%zu, closed:%s)",
           this, readPos_, output_.size(),
           closed_ ? "true" : "false");
  return buf;
}

size_t ByteArrayEndPoint::fill(Buffer* sink) {
  if (closed_) {
    return 0;
  }

  size_t n = sink->size();
  sink->push_back(input_.ref(readPos_));
  n = sink->size() - n;
  readPos_ += n;
  TRACE("%p fill: %zu bytes", this, n);
  return n;
}

size_t ByteArrayEndPoint::flush(const BufferRef& source) {
  TRACE("%p flush: %zu bytes", this, source.size());

  if (closed_) {
    return 0;
  }

  size_t n = output_.size();
  output_.push_back(source);
  return output_.size() - n;
}

size_t ByteArrayEndPoint::flush(int fd, off_t offset, size_t size) {
  output_.reserve(output_.size() + size);

  ssize_t n = pread(fd, output_.end(), size, offset);

  if (n < 0)
    throw std::system_error(errno, std::system_category());

  output_.resize(output_.size() + n);
  return n;
}

void ByteArrayEndPoint::wantFill() {
  if (connection()) {
    TRACE("%p wantFill.", this);
    isBusy_ = true;
    connector_->executor()->execute([this] {
      TRACE("%p wantFill: fillable.", this);
      try {
        connection()->onFillable();
        isBusy_ = false;
      } catch (std::exception& e) {
        connection()->onInterestFailure(e);
        isBusy_ = false;
      }
      if (isClosed()) {
        connector_->release(connection());
      }
    });
  }
}

void ByteArrayEndPoint::wantFlush() {
  if (connection()) {
    TRACE("%p wantFlush.", this);
    isBusy_ = true;
    connector_->executor()->execute([this] {
      TRACE("%p wantFlush: flushable.", this);
      try {
        connection()->onFlushable();
        isBusy_ = false;
      } catch (std::exception& e) {
        connection()->onInterestFailure(e);
        isBusy_ = false;
      }
      if (isClosed()) {
        connector_->release(connection());
      }
    });
  }
}

TimeSpan ByteArrayEndPoint::idleTimeout() {
  return TimeSpan::Zero; // TODO
}

void ByteArrayEndPoint::setIdleTimeout(TimeSpan timeout) {
  // TODO
}

bool ByteArrayEndPoint::isBlocking() const {
  return false;
}

void ByteArrayEndPoint::setBlocking(bool enable) {
  throw std::system_error(ENOTSUP, std::system_category());
}

bool ByteArrayEndPoint::isCorking() const {
  return false;
}

void ByteArrayEndPoint::setCorking(bool /*enable*/) {
}

} // namespace xzero
