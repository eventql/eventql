// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/net/InetEndPoint.h>
#include <cortex-base/net/InetConnector.h>
#include <cortex-base/net/Connection.h>
#include <cortex-base/executor/Executor.h>
#include <cortex-base/logging.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/sysconfig.h>
#include <cortex-base/RefPtr.h>
#include <stdexcept>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#if defined(HAVE_SYS_SENDFILE_H)
#include <sys/sendfile.h>
#endif

namespace cortex {

#define ERROR(msg...) logError("net.InetEndPoint", msg)

#ifndef NDEBUG
#define TRACE(msg...) logTrace("net.InetEndPoint", msg)
#else
#define TRACE(msg...) do {} while (0)
#endif

InetEndPoint::InetEndPoint(int socket,
                           InetConnector* connector,
                           Scheduler* scheduler)
    : EndPoint(),
      connector_(connector),
      scheduler_(scheduler),
      readTimeout_(connector->readTimeout()),
      writeTimeout_(connector->writeTimeout()),
      idleTimeout_(connector->clock(), connector->scheduler()),
      io_(),
      handle_(socket),
      addressFamily_(connector->addressFamily()),
      isCorking_(false) {

  idleTimeout_.setCallback(std::bind(&InetEndPoint::onTimeout, this));
  TRACE("%p ctor", this);
}

InetEndPoint::InetEndPoint(int socket,
                           int addressFamily,
                           TimeSpan readTimeout,
                           TimeSpan writeTimeout,
                           WallClock* clock,
                           Scheduler* scheduler)
    : EndPoint(),
      connector_(nullptr),
      scheduler_(scheduler),
      readTimeout_(readTimeout),
      writeTimeout_(writeTimeout),
      idleTimeout_(clock, scheduler),
      io_(),
      handle_(socket),
      addressFamily_(addressFamily),
      isCorking_(false) {

  idleTimeout_.setCallback(std::bind(&InetEndPoint::onTimeout, this));
  TRACE("%p ctor", this);
}

void InetEndPoint::onTimeout() {
  if (connection()) {
    if (connection()->onReadTimeout()) {
      close();
    }
  }
}

InetEndPoint::~InetEndPoint() {
  TRACE("%p dtor", this);
  if (isOpen()) {
    close();
  }
}

std::pair<IPAddress, int> InetEndPoint::remoteAddress() const {
  if (handle_ < 0)
    RAISE(IllegalStateError, "Invalid socket handle.");

  std::pair<IPAddress, int> result;
  switch (addressFamily()) {
    case AF_INET6: {
      sockaddr_in6 saddr;
      socklen_t slen = sizeof(saddr);
      if (getpeername(handle_, (sockaddr*)&saddr, &slen) == 0) {
        result.first = IPAddress(&saddr);
        result.second = ntohs(saddr.sin6_port);
      }
      break;
    }
    case AF_INET: {
      sockaddr_in saddr;
      socklen_t slen = sizeof(saddr);
      if (getpeername(handle_, (sockaddr*)&saddr, &slen) == 0) {
        result.first = IPAddress(&saddr);
        result.second = ntohs(saddr.sin_port);
      }
      break;
    }
    default:
      RAISE(IllegalStateError, "Invalid address family.");
  }
  return result;
}

std::pair<IPAddress, int> InetEndPoint::localAddress() const {
  if (handle_ < 0)
    RAISE(IllegalStateError, "Invalid socket handle.");

  std::pair<IPAddress, int> result;
  switch (addressFamily()) {
    case AF_INET6: {
      sockaddr_in6 saddr;
      socklen_t slen = sizeof(saddr);

      if (getsockname(handle_, (sockaddr*)&saddr, &slen) == 0) {
        result.first = IPAddress(&saddr);
        result.second = ntohs(saddr.sin6_port);
      }
      break;
    }
    case AF_INET: {
      sockaddr_in saddr;
      socklen_t slen = sizeof(saddr);

      if (getsockname(handle_, (sockaddr*)&saddr, &slen) == 0) {
        result.first = IPAddress(&saddr);
        result.second = ntohs(saddr.sin_port);
      }
      break;
    }
    default:
      break;
  }

  return result;
}

bool InetEndPoint::isOpen() const CORTEX_NOEXCEPT {
  return handle_ >= 0;
}

void InetEndPoint::close() {
  if (isOpen()) {
    ::close(handle_);
    handle_ = -1;

    if (connector_) {
      connector_->onEndPointClosed(this);
    }
  }
}

bool InetEndPoint::isBlocking() const {
  return !(fcntl(handle_, F_GETFL) & O_NONBLOCK);
}

void InetEndPoint::setBlocking(bool enable) {
  TRACE("%p setBlocking(\"%s\")", this, enable ? "blocking" : "nonblocking");
#if 1
  unsigned current = fcntl(handle_, F_GETFL);
  unsigned flags = enable ? (current & ~O_NONBLOCK)
                          : (current | O_NONBLOCK);
#else
  unsigned flags = enable ? fcntl(handle_, F_GETFL) & ~O_NONBLOCK
                          : fcntl(handle_, F_GETFL) | O_NONBLOCK;
#endif

  if (fcntl(handle_, F_SETFL, flags) < 0) {
    RAISE_ERRNO(errno);
  }
}

bool InetEndPoint::isCorking() const {
  return isCorking_;
}

void InetEndPoint::setCorking(bool enable) {
#if defined(TCP_CORK)
  if (isCorking_ != enable) {
    int flag = enable ? 1 : 0;
    if (setsockopt(handle_, IPPROTO_TCP, TCP_CORK, &flag, sizeof(flag)) < 0)
      RAISE_ERRNO(errno);

    isCorking_ = enable;
  }
#endif
}

std::string InetEndPoint::toString() const {
  char buf[32];
  snprintf(buf, sizeof(buf), "InetEndPoint(%d)@%p", handle(), this);
  return buf;
}

size_t InetEndPoint::fill(Buffer* result) {
  result->reserve(result->size() + 1024);
  ssize_t n = read(handle(), result->end(), result->capacity() - result->size());
  TRACE("read(%zu bytes) -> %zi", result->capacity() - result->size(), n);

  if (n < 0) {
    // don't raise on soft errors, such as there is simply no more data to read.
    switch (errno) {
      case EBUSY:
      case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
      case EWOULDBLOCK:
#endif
        break;
      default:
        RAISE_ERRNO(errno);
    }
  } else {
    result->resize(result->size() + n);
  }

  return n;
}

size_t InetEndPoint::flush(const BufferRef& source) {
  ssize_t rv = write(handle(), source.data(), source.size());

  TRACE("flush(%zu bytes) -> %zi", source.size(), rv);

  if (rv < 0)
    RAISE_ERRNO(errno);

  // EOF exception?

  return rv;
}

size_t InetEndPoint::flush(int fd, off_t offset, size_t size) {
#if defined(__APPLE__)
  off_t len = 0;
  int rv = sendfile(fd, handle(), offset, &len, nullptr, 0);
  TRACE("flush(offset:%zu, size:%zu) -> %zi", offset, size, rv);
  if (rv < 0)
    RAISE_ERRNO(errno);

  return len;
#else
  ssize_t rv = sendfile(handle(), fd, &offset, size);
  TRACE("flush(offset:%zu, size:%zu) -> %zi", offset, size, rv);
  if (rv < 0)
    RAISE_ERRNO(errno);

  // EOF exception?

  return rv;
#endif
}

void InetEndPoint::onReadable() CORTEX_NOEXCEPT {
  RefPtr<EndPoint> _guard(this);

  try {
    connection()->onFillable();
  } catch (const std::exception& e) {
    connection()->onInterestFailure(e);
  } catch (...) {
    connection()->onInterestFailure(
        EXCEPTION(RuntimeError, (int) Status::CaughtUnknownExceptionError,
                  StatusCategory::get()));
  }
}

void InetEndPoint::onWritable() CORTEX_NOEXCEPT {
  RefPtr<EndPoint> _guard(this);

  try {
    connection()->onFlushable();
  } catch (const std::exception& e) {
    connection()->onInterestFailure(e);
  } catch (...) {
    connection()->onInterestFailure(
        EXCEPTION(RuntimeError, (int) Status::CaughtUnknownExceptionError,
                  StatusCategory::get()));
  }
}

void InetEndPoint::wantFill() {
  TRACE("%p wantFill()", this);
  // TODO: abstract away the logic of TCP_DEFER_ACCEPT

  //FIXME: idleTimeout_.activate(readTimeout());
  if (!io_) {
    io_ = scheduler_->executeOnReadable(
        handle(),
        std::bind(&InetEndPoint::fillable, this));
  }
}

void InetEndPoint::fillable() {
  RefPtr<EndPoint> _guard(this);

  try {
    io_.reset();
    connection()->onFillable();
  } catch (const std::exception& e) {
    connection()->onInterestFailure(e);
  } catch (...) {
    connection()->onInterestFailure(
        EXCEPTION(RuntimeError, (int) Status::CaughtUnknownExceptionError,
                  StatusCategory::get()));
  }
}

void InetEndPoint::wantFlush() {
  TRACE("%p wantFlush() %s", this, io_.get() ? "again" : "first time");
  //FIXME: idleTimeout_.activate(writeTimeout());

  if (!io_) {
    io_ = scheduler_->executeOnWritable(
        handle(),
        std::bind(&InetEndPoint::flushable, this));
  }
}

void InetEndPoint::flushable() {
  RefPtr<EndPoint> _guard(this);

  try {
    io_.reset();
    connection()->onFlushable();
  } catch (const std::exception& e) {
    connection()->onInterestFailure(e);
  } catch (...) {
    connection()->onInterestFailure(
        EXCEPTION(RuntimeError, (int) Status::CaughtUnknownExceptionError,
                  StatusCategory::get()));
  }
}

TimeSpan InetEndPoint::readTimeout() {
  return readTimeout_;
}

TimeSpan InetEndPoint::writeTimeout() {
  return writeTimeout_;
}

void InetEndPoint::setReadTimeout(TimeSpan timeout) {
  readTimeout_ = timeout;
}

void InetEndPoint::setWriteTimeout(TimeSpan timeout) {
  writeTimeout_ = timeout;
}

Option<IPAddress> InetEndPoint::remoteIP() const {
  return Some(remoteAddress().first);
}

class InetConnectState {
 public:
  std::unique_ptr<InetEndPoint> ep_;
  Promise<std::unique_ptr<InetEndPoint>> promise_;

  InetConnectState(std::unique_ptr<InetEndPoint>&& ep,
                   const Promise<std::unique_ptr<InetEndPoint>>& promise)
      : ep_(std::move(ep)),
        promise_(promise) {}

  void onConnectComplete() {
    int val = 0;
    socklen_t vlen = sizeof(val);
    if (getsockopt(ep_->handle(), SOL_SOCKET, SO_ERROR, &val, &vlen) == 0) {
      TRACE("%p onConnectComplete: connected.", this);
      promise_.success(std::move(ep_));
    } else {
      TRACE("%p onConnectComplete: failure %d. %s", this,
          val, strerror(val));
      promise_.failure(Status::IOError); // dislike: wanna pass errno val here.
    }
    delete this;
  }
};

Future<std::unique_ptr<InetEndPoint>> InetEndPoint::connectAsync(
    const IPAddress& ipaddr, int port,
    TimeSpan timeout, WallClock* clock, Scheduler* scheduler) {

  Promise<std::unique_ptr<InetEndPoint>> promise;
  std::unique_ptr<InetEndPoint> ep;
  int fd = socket(ipaddr.family(), SOCK_STREAM, IPPROTO_TCP);
  if (fd < 0)
    RAISE_ERRNO(errno);

  try {
    TRACE("connectAsync: to %s port %d", ipaddr.str().c_str(), port);
    ep.reset(new InetEndPoint(fd, ipaddr.family(), timeout, timeout,
                               clock, scheduler));
    ep->setBlocking(false);

    switch (ipaddr.family()) {
      case AF_INET: {
        struct sockaddr_in saddr;
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = ipaddr.family();
        saddr.sin_port = htons(port);
        memcpy(&saddr.sin_addr, ipaddr.data(), ipaddr.size());

        // this connect()-call can block if fd is non-blocking
        TRACE("connectAsync: connect(ipv4)");
        if (::connect(fd, (const struct sockaddr*) &saddr, sizeof(saddr)) < 0) {
          if (errno != EINPROGRESS) {
            TRACE("connectAsync: connect() error. %s", strerror(errno));
            promise.failure(Status::IOError); // errno
            return promise.future();
          } else {
            TRACE("connectAsync: backgrounding");
            scheduler->executeOnWritable(fd,
                std::bind(&InetConnectState::onConnectComplete,
                          new InetConnectState(std::move(ep), promise)));
            return promise.future();
          }
        }
        TRACE("connectAsync: synchronous connect");
        break;
      }
      case AF_INET6: {
        struct sockaddr_in6 saddr;
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin6_family = ipaddr.family();
        saddr.sin6_port = htons(port);
        memcpy(&saddr.sin6_addr, ipaddr.data(), ipaddr.size());

        // this connect()-call can block if fd is non-blocking
        if (::connect(fd, (const struct sockaddr*) &saddr, sizeof(saddr)) < 0) {
          if (errno != EINPROGRESS) {
            TRACE("connectAsync: connect() error. %s", strerror(errno));
            RAISE_ERRNO(errno);
          } else {
            TRACE("connectAsync: backgrounding");
            scheduler->executeOnWritable(fd,
                std::bind(&InetConnectState::onConnectComplete,
                          new InetConnectState(std::move(ep), promise)));
            return promise.future();
          }
        }
        break;
      }
      default: {
        RAISE(NotImplementedError);
      }
    }

    TRACE("connectAsync: connected instantly");
    promise.success(std::move(ep));
    return promise.future();
  } catch (...) {
    ::close(fd);
    throw;
  }
}

std::unique_ptr<InetEndPoint> InetEndPoint::connect(
    const IPAddress& ipaddr, int port,
    TimeSpan timeout, WallClock* clock, Scheduler* scheduler) {
  std::unique_ptr<InetEndPoint> ep =
      std::move(connectAsync(ipaddr, port, timeout, clock, scheduler).get());
  ep->setBlocking(true);
  return ep;
}

} // namespace cortex
