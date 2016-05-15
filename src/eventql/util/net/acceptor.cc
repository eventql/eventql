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

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "acceptor.h"
#include "eventloop.h"
#include <eventql/util/exception.h>

namespace stxmetric {
namespace ev {

Acceptor::Acceptor(EventLoop* ev_loop) : ev_loop_(ev_loop) {}

void Acceptor::listen(int port, CallbackInterface* handler) {
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  int ssock = socket(AF_INET, SOCK_STREAM, 0);
  if (ssock == -1) {
    RAISE_ERRNO(kIOError, "socket() creation failed");
    return;
  }

  int opt = 1;
  if (setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    RAISE_ERRNO(kIOError, "setsockopt(SO_REUSEADDR) failed");
    return;
  }

#ifdef SOL_TCP
  if (setsockopt(ssock, SOL_TCP, TCP_QUICKACK, &opt, sizeof(opt)) < 0) {
    RAISE_ERRNO(kIOError, "setsockopt(TCP_QUICKACK) failed");
    return;
  }

  if (setsockopt(ssock, SOL_TCP, TCP_DEFER_ACCEPT, &opt, sizeof(opt)) < 0) {
    RAISE_ERRNO(kIOError, "setsockopt(TCP_DEFER_ACCEPT) failed");
    return;
  }
#endif

  if (bind(
      ssock,
      (struct sockaddr *)
      &server_addr, 
      sizeof(server_addr)) == -1) {
    RAISE_ERRNO(kIOError, "bind() failed");
    return;
  }

  if (::listen(ssock, 1024) == -1) {
    RAISE_ERRNO(kIOError, "listen() failed");
    return;
  }

  int flags = fcntl(ssock, F_GETFL, 0);
  flags = flags | O_NONBLOCK;

  if (fcntl(ssock, F_SETFL, flags) != 0) {
    RAISE(kIOError, "fnctl() failed");
  }

  auto handler_ptr = new HandlerRef(ssock, handler);
  ev_loop_->watch(ssock, EventLoop::EV_READABLE, handler_ptr);
  handlers_.emplace_back(handler_ptr);
}

Acceptor::HandlerRef::HandlerRef(
    int ssock,
    Acceptor::CallbackInterface* handler) :
    ssock_(ssock),
    handler_(handler) {}

Acceptor::HandlerRef::~HandlerRef() {
  close(ssock_);
}

void Acceptor::HandlerRef::onEvent(
    EventLoop* loop,
    int fd,
    EventLoop::kInterestType ev) {
  int conn_fd = accept(ssock_, NULL, NULL);

  if (conn_fd == -1) {
    RAISE_ERRNO(kIOError, "accept() failed");
    return;
  }

  int flags = fcntl(conn_fd, F_GETFL, 0);
  flags &= ~O_NONBLOCK;

  if (fcntl(conn_fd, F_SETFL, flags) != 0) {
    RAISE(kIOError, "fnctl() failed");
  }

  handler_->onConnection(conn_fd);
  loop->watch(fd, EventLoop::EV_READABLE, this);
}

}
}
