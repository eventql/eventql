#include <xzero/net/InetEndPoint.h>
#include <xzero/net/InetConnector.h>
#include <xzero/net/Connection.h>
#include <xzero/executor/Executor.h>
#include <xzero/io/SelectionKey.h>
#include <xzero/Buffer.h>
#include <xzero/sysconfig.h>
#include <system_error>
#include <stdexcept>
#include <netinet/tcp.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>

namespace xzero {

InetEndPoint::InetEndPoint(int socket, InetConnector* connector)
    : EndPoint(),
      connector_(connector),
      handle_(socket),
      isCorking_(false),
      isBusy_(false) {
}

InetEndPoint::~InetEndPoint() {
  if (isOpen()) {
    close();
  }
}

std::pair<IPAddress, int> InetEndPoint::remoteAddress() const {
  std::pair<IPAddress, int> result;
  // TODO
  return result;
}

std::pair<IPAddress, int> InetEndPoint::localAddress() const {
  std::pair<IPAddress, int> result;
  // TODO
  return result;
}

int InetEndPoint::handle() const noexcept {
  return handle_;
}

Selector* InetEndPoint::selector() const noexcept {
  return connector_->selector();
}

void InetEndPoint::setHandle(int handle) noexcept {
  handle_ = handle;
}

bool InetEndPoint::isOpen() const noexcept {
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
  unsigned flags = enable ? fcntl(handle_, F_GETFL) & ~O_NONBLOCK
                          : fcntl(handle_, F_GETFL) | O_NONBLOCK;

  if (fcntl(handle_, F_SETFL, flags) < 0) {
    throw std::system_error(errno, std::system_category());
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
      throw std::system_error(errno, std::system_category());

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

  if (n < 0)
    throw std::system_error(errno, std::system_category());

  result->resize(result->size() + n);
  return n;
}

size_t InetEndPoint::flush(const BufferRef& source) {
  ssize_t rv = write(handle(), source.data(), source.size());

  if (rv < 0)
    throw std::system_error(errno, std::system_category());

  // EOF exception?

  return rv;
}

size_t InetEndPoint::flush(int fd, off_t offset, size_t size) {
  ssize_t rv = sendfile(handle(), fd, &offset, size);

  if (rv < 0)
    throw std::system_error(errno, std::system_category());

  // EOF exception?

  return rv;
}

void InetEndPoint::wantFill() {
  // TODO: abstract away the logic of TCP_DEFER_ACCEPT

  if (selector()) {
    selectionKey_ = registerSelectable(READ);
  } else {
    // XXX use executor here to flatten callstack on repeative wantFill()
    connector_->executor()->execute([this]() {
      isBusy_ = true;
      try {
        connection()->onFillable();
        isBusy_ = false;
      } catch (const std::exception& e) {
        connection()->onInterestFailure(e);
        isBusy_ = false;
      }
      if (isClosed()) {
        connector_->release(connection());
      }
    });
  }
}

void InetEndPoint::onSelectable() noexcept {
  // printf("InetEndPoint.onSelectable()%s%s\n",
  //     selectionKey_->isReadable() ? " read" : "",
  //     selectionKey_->isWriteable() ? " write" : "");

  isBusy_ = true;

  try {
    if (selectionKey_->isReadable()) {
      connection()->onFillable();
    }

    if (selectionKey_->isWriteable()) {
      connection()->onFlushable();
    }

    isBusy_ = false;
  } catch (const std::exception& e) {
    connection()->onInterestFailure(e);
    isBusy_ = false;
  }

  if (isClosed()) {
    connector_->release(connection());
  }
}

void InetEndPoint::wantFlush() {
  if (selector()) {
    if (selectionKey_)
      selectionKey_->change(WRITE);
    else
      selectionKey_ = registerSelectable(WRITE);
  } else {
    connection()->onFlushable();
  }
}

TimeSpan InetEndPoint::idleTimeout() {
  return TimeSpan::Zero; // TODO
}

void InetEndPoint::setIdleTimeout(TimeSpan timeout) {
  // TODO
}

} // namespace xzero
