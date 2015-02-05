// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/net/InetEndPoint.h>
#include <xzero/net/InetConnector.h>
#include <xzero/net/Connection.h>
#include <xzero/executor/Executor.h>
#include <xzero/logging/LogSource.h>
#include <xzero/RuntimeError.h>
#include <xzero/Buffer.h>
#include <xzero/sysconfig.h>
#include <xzero/RuntimeError.h>
#include <stdexcept>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#if defined(HAVE_SYS_SENDFILE_H)
#include <sys/sendfile.h>
#endif

namespace xzero {

static LogSource inetEndPointLogger("net.InetEndPoint");
#ifndef NDEBUG
#define TRACE(msg...) do { inetEndPointLogger.trace(msg); } while (0)
#define ERROR(msg...) do { inetEndPointLogger.error(msg); } while (0)
#else
#define TRACE(msg...) do {} while (0)
#define ERROR(msg...) do {} while (0)
#endif

InetEndPoint::InetEndPoint(int socket,
                           InetConnector* connector,
                           Scheduler* scheduler)
    : EndPoint(),
      connector_(connector),
      scheduler_(scheduler),
      idleTimeout_(connector->clock(), connector->scheduler()),
      handle_(socket),
      isCorking_(false),
      isBusy_(0) {

  idleTimeout_.setCallback(std::bind(&InetEndPoint::onTimeout, this));
  idleTimeout_.setTimeout(connector->idleTimeout());
}

void InetEndPoint::onTimeout() {
  if (connection()) {
    if (connection()->onReadTimeout()) {
      close();
    }
  }
}

InetEndPoint::~InetEndPoint() {
  if (isOpen()) {
    close();
  }
}

std::pair<IPAddress, int> InetEndPoint::remoteAddress() const {
  if (handle_ < 0)
    throw RUNTIME_ERROR("Illegal State. Socket is closed.");

  std::pair<IPAddress, int> result;
  switch (connector_->addressFamily()) {
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
      throw RUNTIME_ERROR("Illegal State. IPAddress.addressFamily?");
  }
  return result;
}

std::pair<IPAddress, int> InetEndPoint::localAddress() const {
  if (handle_ < 0)
    throw RUNTIME_ERROR("Illegal State. Socket is closed.");

  std::pair<IPAddress, int> result;
  switch (connector_->addressFamily()) {
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

bool InetEndPoint::isOpen() const XZERO_NOEXCEPT {
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
    throw SYSTEM_ERROR(errno);
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
      throw SYSTEM_ERROR(errno);

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
        throw SYSTEM_ERROR(errno);
    }
  } else {
    result->resize(result->size() + n);
  }

  return n;
}

size_t InetEndPoint::flush(const BufferRef& source) {
  ssize_t rv = write(handle(), source.data(), source.size());

  if (rv < 0)
    throw SYSTEM_ERROR(errno);

  // EOF exception?

  return rv;
}

size_t InetEndPoint::flush(int fd, off_t offset, size_t size) {
#if defined(__APPLE__)
  off_t len = 0;
  int rv = sendfile(fd, handle(), offset, &len, nullptr, 0);
  if (rv < 0)
    throw SYSTEM_ERROR(errno);

  return len;
#else
  ssize_t rv = sendfile(handle(), fd, &offset, size);
  if (rv < 0)
    throw SYSTEM_ERROR(errno);

  // EOF exception?

  return rv;
#endif
}

void InetEndPoint::onReadable() {
  try {
    /*lock guard*/ {
      BusyGuard _busyGuard(this);
      connection()->onFillable();
    }

    if (!isBusy() && isClosed()) {
      connector_->release(connection());
    }
  } catch (const std::exception& e) {
    connection()->onInterestFailure(e);
  } catch (...) {
    connection()->onInterestFailure(RUNTIME_ERROR("Unhandled unknown exception caught."));
  }
}

void InetEndPoint::onWritable() XZERO_NOEXCEPT {
  try {
    /*lock guard*/ {
      BusyGuard _busyGuard(this);
      connection()->onFlushable();
    }

    if (!isBusy() && isClosed()) {
      connector_->release(connection());
    }
  } catch (const std::exception& e) {
    connection()->onInterestFailure(e);
  } catch (...) {
    connection()->onInterestFailure(RUNTIME_ERROR("Unhandled unknown exception caught."));
  }
}

void InetEndPoint::wantFill() {
  TRACE("%p wantFill()", this);
  // TODO: abstract away the logic of TCP_DEFER_ACCEPT

  //idleTimeout_.activate();
  scheduler_->executeOnReadable(handle(),
                                std::bind(&InetEndPoint::fillable, this));
}

void InetEndPoint::fillable() {
  {
    BusyGuard _busyGuard(this);
    try {
      connection()->onFillable();
    } catch (const std::exception& e) {
      connection()->onInterestFailure(e);
    } catch (...) {
      connection()->onInterestFailure(RUNTIME_ERROR("Unhandled unknown exception caught."));
    }
  }
  if (!isBusy() && isClosed()) {
    connector_->release(connection());
  }
}

void InetEndPoint::wantFlush() {
  TRACE("%p wantFlush()", this);
  //idleTimeout_.activate();
  scheduler_->executeOnWritable(handle(),
                                std::bind(&InetEndPoint::flushable, this));
}

void InetEndPoint::flushable() {
  connector_->executor()->execute([this]() {
    /*lock guard*/ {
      BusyGuard _busyGuard(this);
      try {
        connection()->onFlushable();
      } catch (const std::exception& e) {
        connection()->onInterestFailure(e);
      } catch (...) {
        connection()->onInterestFailure(RUNTIME_ERROR("Unhandled unknown exception caught."));
      }
    }
    if (!isBusy() && isClosed()) {
      connector_->release(connection());
    }
  });
}

TimeSpan InetEndPoint::idleTimeout() {
  return idleTimeout_.timeout();
}

void InetEndPoint::setIdleTimeout(TimeSpan timeout) {
  idleTimeout_.setTimeout(timeout);
}

} // namespace xzero
