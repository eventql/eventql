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

#define TRACE(msg...) { \
  fprintf(stderr, "SslEndPoint: " msg); \
  fprintf(stderr, "\n"); \
}

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
 * - wantFlush/wantFill:
 *    1.) desire_ to Desire::WantFlush || Desire::WantFill
 *    2.) trigger select() based on SSL_ERROR_WANT_READ || SSL_ERROR_WANT_WRITE
 *    3.) will get called back on I/O notification to
 *        SslEndPoint::onRead/onWrite to repeat/complete SSL_read/SSL_write
 *
 */

// {{{ BIO_METHOD impl
static int ep_bio_write(BIO* bio, const char* buf, int len) {
  TRACE("ep_bio_write(%p, %p %d)", bio, buf, len);
  SslEndPoint* ep = (SslEndPoint*) bio->ptr;
  ssize_t n = ::write(ep->handle(), buf, len);
  return 0;
}

static int ep_bio_read(BIO* bio, char* buf, int len) {
  TRACE("ep_bio_read(%p, %p, %d)", bio, buf, len);

  SslEndPoint* ep = (SslEndPoint*) bio->ptr;
  ssize_t n = ::read(ep->handle(), buf, len);
  return n;
}

static int ep_bio_puts(BIO* bio, const char* str) {
  TRACE("ep_bio_puts(%p)", bio);
  return ep_bio_write(bio, str, strlen(str));
}

static int ep_bio_gets(BIO* bio, char* str, int len) {
  TRACE("ep_bio_gets(%p)", bio);
  return -1;
}

static long ep_bio_ctrl(BIO* bio, int cmd, long num, void* ptr) {
  TRACE("ep_bio_ctrl(%p, %d, %ld, %p)", bio, cmd, num, ptr);

  switch (cmd) {
    case BIO_CTRL_FLUSH:
      return 1;
    case BIO_CTRL_WPENDING:
      // write pending
    case BIO_CTRL_PENDING:
      // read pending
    case BIO_CTRL_EOF:
      return 0;
    case BIO_CTRL_RESET:
    default:
      return 0;
  }
}

static int ep_bio_ctor(BIO* bio) {
  TRACE("ep_bio_ctor(%p)", bio);
  bio->shutdown = 1;
  bio->init = 1;
  bio->num = -1; // arbitrary number (ex: the file descriptor).
  bio->ptr = nullptr; // shall be SslEndPoint

  return 1;
}

static int ep_bio_dtor(BIO* bio) {
  TRACE("ep_bio_dtor(%p)", bio);
  if (bio == nullptr)
    return 0;

  return 1;
}

static BIO_METHOD ep_bio = {
  42 | BIO_TYPE_SOURCE_SINK,
  "endpoint_bio",
  &ep_bio_write,
  &ep_bio_read,
  &ep_bio_puts,
  &ep_bio_gets,
  &ep_bio_ctrl,
  &ep_bio_ctor,
  &ep_bio_dtor,
};
// }}}

SslEndPoint::SslEndPoint(
    int socket, SslConnector* connector, Scheduler* scheduler)
    : handle_(socket),
      connector_(connector),
      scheduler_(scheduler),
      bio_(BIO_new(&ep_bio)),
      ssl_(),
      io_(),
      idleTimeout_(connector->clock(), scheduler),
      readBuffer_(),
      writeBuffer_() {
  TRACE("%p SslEndPoint() ctor", this);

  bio_->ptr = this;
  bio_->num = socket;
}

SslEndPoint::~SslEndPoint() {
  TRACE("%p ~SslEndPoint() dtor", this);
  if (handle_ >= 0)
    ::close(handle_);
}

bool SslEndPoint::isOpen() const {
  return handle_ >= 0;
}

void SslEndPoint::close() {
  int rv = SSL_shutdown(ssl_);
  TRACE("%p close() (SSL_shutdown() -> %d", this, rv);
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
  TRACE("%p fill(Buffer)", this);

  int rv = SSL_read(ssl_, sink->end(), sink->capacity() - sink->size());
  if (rv > 0) {
    sink->resize(sink->size() + rv);
    return rv;
  }

  switch (SSL_get_error(ssl_, rv)) {
    case SSL_ERROR_WANT_READ:
      bioDesire_ = Desire::Read;
      break;
    case SSL_ERROR_WANT_WRITE:
      bioDesire_ = Desire::Write;
      break;
    default:
      throw RUNTIME_ERROR("SSL write error");
  }
  errno = EAGAIN;
  return 0;
}

size_t SslEndPoint::flush(const BufferRef& source) {
  TRACE("%p flush(BufferRef, @%p, %zu bytes)",
        this, source.data(), source.size());

  int rv = SSL_write(ssl_, source.data(), source.size());
  if (rv > 0) {
    return rv;
  }

  switch (SSL_get_error(ssl_, rv)) {
    case SSL_ERROR_WANT_READ:
      bioDesire_ = Desire::Read;
      break;
    case SSL_ERROR_WANT_WRITE:
      bioDesire_ = Desire::Read;
      break;
    default:
      throw RUNTIME_ERROR("SSL write error");
  }
  errno = EAGAIN;
  return 0;
}

size_t SslEndPoint::flush(int fd, off_t offset, size_t size) {
  Buffer buf;
  buf.reserve(size);
  ssize_t rv = ::read(fd, buf.data(), buf.capacity());
  if (rv < 0) {
    switch (errno) {
      case EBUSY:
      case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
      case EWOULDBLOCK:
#endif
        return 0;
      default:
        throw SYSTEM_ERROR(errno);
    }
  }

  buf.resize(static_cast<size_t>(rv));
  return flush(buf);
}

void SslEndPoint::wantFill() {
  if (io_)
    return;

  switch (bioDesire_) {
    case Desire::None:
    case Desire::Read:
      io_ = scheduler_->executeOnWritable(
          handle(),
          std::bind(&SslEndPoint::fillable, this));
      break;
    case Desire::Write:
      io_ = scheduler_->executeOnReadable(
          handle(),
          std::bind(&SslEndPoint::fillable, this));
      break;
  }
}

void SslEndPoint::fillable() {
  RefPtr<EndPoint> _guard(this);
  try {
    bioDesire_ = Desire::None;
    io_.reset();
    connection()->onFillable();
  } catch (const std::exception& e) {
    connection()->onInterestFailure(e);
  } catch (...) {
    connection()->onInterestFailure(
        RUNTIME_ERROR("Unhandled unknown exception caught."));
  }
}

void SslEndPoint::wantFlush() {
  if (io_)
    return;

  switch (bioDesire_) {
    case Desire::None:
    case Desire::Read:
      io_ = scheduler_->executeOnWritable(
          handle(),
          std::bind(&SslEndPoint::flushable, this));
      break;
    case Desire::Write:
      io_ = scheduler_->executeOnReadable(
          handle(),
          std::bind(&SslEndPoint::flushable, this));
      break;
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

void SslEndPoint::flushable() {
  RefPtr<EndPoint> _guard(this);
  try {
    bioDesire_ = Desire::None;
    io_.reset();
    connection()->onFlushable();
  } catch (const std::exception& e) {
    connection()->onInterestFailure(e);
  } catch (...) {
    connection()->onInterestFailure(
        RUNTIME_ERROR("Unhandled unknown exception caught."));
  }
}

void SslEndPoint::onShutdown() {
  int rv = SSL_shutdown(ssl_);
  TRACE("%p close() (SSL_shutdown() -> %d", this, rv);
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

} // namespace xzero
