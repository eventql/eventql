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
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <eventql/util/exception.h>
#include <eventql/util/net/udpserver.h>

namespace net {

UDPServer::UDPServer(
    TaskScheduler* server_scheduler,
    TaskScheduler* callback_scheduler) :
    server_scheduler_(server_scheduler),
    callback_scheduler_(callback_scheduler) {}

UDPServer::~UDPServer() {
  // FIXPAUL cancel pending task
  close(ssock_);
}

void UDPServer::onMessage(
    std::function<void (const Buffer&)> callback) {
  callback_ = callback; // FIXPAUL lock or doc
}

void UDPServer::listen(int port) {
  ssock_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (ssock_ == 0) {
    RAISE(kIOError, "create socket() failed");
  }

  int opt = 1;
  if (setsockopt(ssock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    RAISE_ERRNO(kIOError, "setsockopt(SO_REUSEADDR) failed");
    return;
  }

  struct sockaddr_in addr;
  memset((char *) &addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  if (bind(ssock_, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    RAISE_ERRNO(kIOError, "bind() failed");
  }

  int flags = fcntl(ssock_, F_GETFL, 0);
  flags = flags | O_NONBLOCK;

  if (fcntl(ssock_, F_SETFL, flags) != 0) {
    RAISE_ERRNO(kIOError, "fnctl(%i) failed", ssock_);
  }

  server_scheduler_->runOnReadable(
      std::bind(&UDPServer::messageReceived, this),
      ssock_);
}

void UDPServer::messageReceived() {
  struct sockaddr_in other_addr;
  socklen_t other_addr_len = sizeof(other_addr);

  char buf[65535];
  auto buf_len = recvfrom(
      ssock_,
      buf,
      sizeof(buf),
      0,
      (struct sockaddr *) &other_addr,
      &other_addr_len);

  server_scheduler_->runOnReadable(
      std::bind(&UDPServer::messageReceived, this),
      ssock_);

  if (buf_len < 0) {
    RAISE_ERRNO(kIOError, "read(%i) failed", ssock_);
  }

  if (callback_) {
    Buffer msg(buf, buf_len);
    callback_scheduler_->run([msg, this] () { this->callback_(msg); });
  }
}

}

