/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <unistd.h>
#include "eventql/util/exception.h"
#include "eventql/util/net/udpsocket.h"

namespace net {

UDPSocket::UDPSocket() {
  fd_ = socket(AF_INET, SOCK_DGRAM, 0);

  if (fd_ == -1) {
    RAISE_ERRNO(kIOError, "socket() creation failed");
  }
}

UDPSocket::~UDPSocket() {
  close(fd_);
}

void UDPSocket::sendTo(const Buffer& pkt, const InetAddr& addr) {
  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(addr.port());
  inet_aton(addr.ip().c_str(), &(saddr.sin_addr));
  memset(&(saddr.sin_zero), 0, 8);

  auto res = sendto(
      fd_,
      pkt.data(),
      pkt.size(),
      0,
      (struct sockaddr *) &saddr,
      sizeof(saddr));

  if (res < 0) {
    RAISE_ERRNO(kIOError, "sendTo() failed");
  }
}

}
