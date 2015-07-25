// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/net/UdpClient.h>
#include <cortex-base/net/IPAddress.h>
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

UdpClient::UdpClient(const IPAddress& ipaddr, int port)
    : socket_(-1),
      addressFamily_(ipaddr.family()),
      sockAddr_(nullptr),
      sockAddrLen_(0)
{
  switch (ipaddr.family()) {
    case IPAddress::V4:
      sockAddrLen_ = sizeof(sockaddr_in);

      sockAddr_ = malloc(sockAddrLen_);
      if (!sockAddr_)
        RAISE(MallocError);

      memset(sockAddr_, 0, sockAddrLen_);

      ((sockaddr_in*)sockAddr_)->sin_port = htons(port);
      ((sockaddr_in*)sockAddr_)->sin_family = AF_INET;
      memcpy(&((sockaddr_in*)sockAddr_)->sin_addr,
             ipaddr.data(), ipaddr.size());
      break;
    case IPAddress::V6:
      sockAddrLen_ = sizeof(sockaddr_in6);

      sockAddr_ = malloc(sockAddrLen_);
      if (!sockAddr_)
        RAISE(MallocError);

      memset(sockAddr_, 0, sockAddrLen_);

      ((sockaddr_in6*)sockAddr_)->sin6_port = htons(port);
      ((sockaddr_in6*)sockAddr_)->sin6_family = AF_INET6;
      memcpy(&((sockaddr_in6*)sockAddr_)->sin6_addr,
             ipaddr.data(), ipaddr.size());
      break;
    default:
      RAISE(IllegalStateError);
  }

  socket_ = ::socket(ipaddr.family(), SOCK_DGRAM, 0);
}

UdpClient::~UdpClient() {
  if (socket_ >= 0) {
    ::close(socket_);
  }
  free(sockAddr_);
}

size_t UdpClient::send(const BufferRef& message) {
  const int flags = 0;
  ssize_t n;

  do {
    n = sendto(
        socket_,
        message.data(),
        message.size(),
        flags,
        (sockaddr*) sockAddr_,
        sockAddrLen_);
  } while (n < 0 && errno == EINTR);

  if (n < 0)
    RAISE_ERRNO(errno);

  return n;
}

size_t UdpClient::receive(Buffer* message) {
  int n;

  message->reserve(65536);
  socklen_t socklen = (socklen_t) sockAddrLen_;

  do {
    n = recvfrom(
        socket_,
        message->data(),
        message->capacity(),
        0,
        (sockaddr*) sockAddr_,
        &socklen);
  } while (n < 0 && errno == EINTR);

  if (n < 0)
    RAISE_ERRNO(errno);

  return static_cast<size_t>(n);
}

} // namespace cortex
