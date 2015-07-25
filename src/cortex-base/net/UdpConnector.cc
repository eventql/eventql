// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/net/UdpConnector.h>
#include <cortex-base/net/UdpEndPoint.h>
#include <cortex-base/net/IPAddress.h>
#include <cortex-base/executor/Scheduler.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/logging.h>

#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

namespace cortex {

UdpConnector::UdpConnector(
    const std::string& name,
    DatagramHandler handler,
    Executor* executor,
    Scheduler* scheduler,
    const IPAddress& ipaddr, int port,
    bool reuseAddr, bool reusePort)
    : DatagramConnector(name, handler, executor),
      scheduler_(scheduler),
      socket_(-1),
      addressFamily_(0) {
  open(ipaddr, port, reuseAddr, reusePort);

  BUG_ON(executor == nullptr);
  BUG_ON(scheduler == nullptr);
}

UdpConnector::~UdpConnector() {
  if (isStarted()) {
    stop();
  }

  if (socket_ >= 0) {
    ::close(socket_);
    socket_ = -1;
  }
}

void UdpConnector::start() {
  notifyOnEvent();
}

bool UdpConnector::isStarted() const {
  return schedulerHandle_.get() != nullptr;
}

void UdpConnector::stop() {
  if (!isStarted())
    RAISE(IllegalStateError);

  if (schedulerHandle_) {
    schedulerHandle_->cancel();
    schedulerHandle_ = nullptr;
  }
}

void UdpConnector::open(
    const IPAddress& ipaddr, int port,
    bool reuseAddr, bool reusePort) {

  socket_ = ::socket(ipaddr.family(), SOCK_DGRAM, 0);
  addressFamily_ = ipaddr.family();

  if (socket_ < 0)
    RAISE_ERRNO(errno);

  if (reusePort) {
    int rc = 1;
    if (::setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &rc, sizeof(rc)) < 0) {
      RAISE_ERRNO(errno);
    }
  }

  if (reuseAddr) {
    int rc = 1;
    if (::setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &rc, sizeof(rc)) < 0) {
      RAISE_ERRNO(errno);
    }
  }

  // bind(ipaddr, port);
  char sa[sizeof(sockaddr_in6)];
  socklen_t salen = ipaddr.size();

  switch (ipaddr.family()) {
    case IPAddress::V4:
      salen = sizeof(sockaddr_in);
      ((sockaddr_in*)sa)->sin_port = htons(port);
      ((sockaddr_in*)sa)->sin_family = AF_INET;
      memcpy(&((sockaddr_in*)sa)->sin_addr, ipaddr.data(), ipaddr.size());
      break;
    case IPAddress::V6:
      salen = sizeof(sockaddr_in6);
      ((sockaddr_in6*)sa)->sin6_port = htons(port);
      ((sockaddr_in6*)sa)->sin6_family = AF_INET6;
      memcpy(&((sockaddr_in6*)sa)->sin6_addr, ipaddr.data(), ipaddr.size());
      break;
    default:
      RAISE_ERRNO(EINVAL);
  }

  int rv = ::bind(socket_, (sockaddr*)sa, salen);
  if (rv < 0)
    RAISE_ERRNO(errno);
}

void UdpConnector::notifyOnEvent() {
  logTrace("UdpConnector", "notifyOnEvent()");
  schedulerHandle_ = scheduler_->executeOnReadable(
      socket_,
      std::bind(&UdpConnector::onMessage, this));
}

void UdpConnector::onMessage() {
  logTrace("UdpConnector", "onMessage");

  socklen_t remoteAddrLen;
  sockaddr* remoteAddr;
  sockaddr_in sin4;
  sockaddr_in6 sin6;

  switch (addressFamily_) {
    case IPAddress::V4:
      remoteAddr = (sockaddr*) &sin4;
      remoteAddrLen = sizeof(sin4);
      break;
    case IPAddress::V6:
      remoteAddr = (sockaddr*) &sin6;
      remoteAddrLen = sizeof(sin6);
      break;
    default:
      RAISE(IllegalStateError);
  }

  Buffer message;
  message.reserve(65535);

  notifyOnEvent();

  int n;
  do {
    n = recvfrom(
        socket_,
        message.data(),
        message.capacity(),
        0,
        remoteAddr,
        &remoteAddrLen);
  } while (n < 0 && errno == EINTR);

  if (n < 0)
    RAISE_ERRNO(errno);

  if (handler_) {
    message.resize(n);
    RefPtr<DatagramEndPoint> client(new UdpEndPoint(
        this, std::move(message), remoteAddr, remoteAddrLen));
    executor_->execute(std::bind(handler_, client));
  } else {
    logTrace("UdpConnector",
             "ignoring incoming message of %i bytes. No handler set.", n);
  }
}

} // namespace cortex
