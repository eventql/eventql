// This file is part of the "x0" project
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#include <xzero/net/SslEndPoint.h>
#include <xzero/net/SslConnector.h>
#include <xzero/net/Connection.h>
#include <xzero/executor/Scheduler.h>
#include <xzero/RuntimeError.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <unistd.h>

namespace xzero {

/* XXX brain dump XXX
 *
 * - wantFlush/flush/wantFill/fill has nothing to do with the actual
 *   SSL write/read operations; they do just fill buffers and
 *   trigger lower layer SSL I/O operations
 *
 * - when lower layer SSL I/O completes it checks if an wantFlush/wantFill
 *   was issued and will then callback connection->onFlushable | onFillable.
 *
 * - SSL_write/SSL_read must either write everything or nothing at all.
 *   The underlying BIO must manage partial writes transparently.
 *
 */

SslEndPoint::SslEndPoint(
    int socket, SslConnector* connector, Scheduler* scheduler)
    : handle_(socket),
      connector_(connector),
      scheduler_(scheduler),
      ssl_(),
      writeBio_(),
      readBio_(),
      io_(),
      idleTimeout_(connector->clock(), scheduler),
      readBuffer_(),
      writeBuffer_() {
}

SslEndPoint::~SslEndPoint() {
  if (handle_ >= 0)
    ::close(handle_);
}

bool SslEndPoint::isOpen() const {
  return handle_ >= 0;
}

void SslEndPoint::close() {
  int rv = SSL_shutdown(ssl_);
  switch (rv) {
    case -1:
      // fatal
      break;
    case 0:
      // repeat call a 2nd time
      break;
    case 1:
      // success
      break;
  }
}

size_t SslEndPoint::fill(Buffer* sink) {
  // TODO: fill readBuffer_
  return 0;
}

size_t SslEndPoint::flush(const BufferRef& source) {
  if (!writeBuffer_.empty()) {
    // queue up
    writeBuffer_.push_back(source);
    return 0;
  }

  writeBuffer_.push_back(source);
  int rv = SSL_write(ssl_, writeBuffer_.data(), writeBuffer_.size());
  if (rv <= 0) {
    switch (SSL_get_error(ssl_, rv)) {
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        // write pending
        errno = EAGAIN;
        return -1;
      default:
        throw RUNTIME_ERROR("SSL write error");
    }
  }
  return source.size();
}

size_t SslEndPoint::flush(int fd, off_t offset, size_t size) {
  // TODO: fill writeBuffer_
  onWrite();
  return 0;
}

void SslEndPoint::wantFill() {
  if (!readBuffer_.empty()) {
    connection()->onFillable();
  } else {
    io_ = scheduler_->executeOnReadable(
        handle(),
        std::bind(&SslEndPoint::fillable, this));
  }
}

void SslEndPoint::fillable() {
  RefPtr<EndPoint> _guard(this);

  try {
    io_.reset();
    connection()->onFillable();
  } catch (const std::exception& e) {
    connection()->onInterestFailure(e);
  } catch (...) {
    connection()->onInterestFailure(RUNTIME_ERROR("Unhandled unknown exception caught."));
  }
}


void SslEndPoint::wantFlush() {
  if (!writeBuffer_.empty()) {
    connection()->onFlushable();
  } else {
    io_ = scheduler_->executeOnWritable(
        handle(),
        std::bind(&SslEndPoint::onWrite, this));
  }
}

TimeSpan SslEndPoint::idleTimeout() {
  return idleTimeout_.timeout();
}

void SslEndPoint::setIdleTimeout(TimeSpan timeout) {
  // TODO
}

bool SslEndPoint::isBlocking() const {
  return false; // TODO
}

void SslEndPoint::setBlocking(bool enable) {
}

bool SslEndPoint::isCorking() const {
  return false; // TODO
}

void SslEndPoint::setCorking(bool enable) {
  // TODO
}

std::string SslEndPoint::toString() const {
  char buf[64];
  int n = snprintf(buf, sizeof(buf), "SslEndPoint(fd=%d)", handle_);
  return std::string(buf, n);
}

void SslEndPoint::onHandshake() {
  int rv = SSL_accept(ssl_);
  if (rv <= 0) {
    switch (SSL_get_error(ssl_, rv)) {
      case SSL_ERROR_WANT_READ:
        scheduler_->executeOnReadable(
            handle(), std::bind(&SslEndPoint::onHandshake, this));
        break;
      case SSL_ERROR_WANT_WRITE:
        scheduler_->executeOnWritable(
            handle(), std::bind(&SslEndPoint::onHandshake, this));
        break;
      default:
        throw RUNTIME_ERROR("SSL handshake error");
    }
  } else {
    // notify connection layer that we're ready to rumble
    connection()->onOpen();
  }
}

void SslEndPoint::onRead() {
  int rv = SSL_read(ssl_, readBuffer_.data(), readBuffer_.capacity());

  if (rv <= 0) {
    switch (SSL_get_error(ssl_, rv)) {
      case SSL_ERROR_WANT_READ:
        scheduler_->executeOnReadable(
            handle(), std::bind(&SslEndPoint::onRead, this));
        break;
      case SSL_ERROR_WANT_WRITE:
        scheduler_->executeOnWritable(
            handle(), std::bind(&SslEndPoint::onRead, this));
        break;
      default:
        throw RUNTIME_ERROR("SSL handshake error");
    }
  } else {
    connection()->onFillable();
  }
}

void SslEndPoint::onWrite() {
  int rv = SSL_write(ssl_, writeBuffer_.data(), writeBuffer_.size());

  if (rv <= 0) {
    switch (SSL_get_error(ssl_, rv)) {
      case SSL_ERROR_WANT_READ:
        scheduler_->executeOnReadable(
            handle(), std::bind(&SslEndPoint::onWrite, this));
        break;
      case SSL_ERROR_WANT_WRITE:
        scheduler_->executeOnWritable(
            handle(), std::bind(&SslEndPoint::onWrite, this));
        break;
      default:
        throw RUNTIME_ERROR("SSL handshake error");
    }
  } else {
    // All pending write-data successfully transmitted.
    // Notify connection layer that we fullfilled the wantFlush-interest now.
    connection()->onFlushable();
  }
}

void SslEndPoint::onShutdown() {
}

} // namespace xzero
