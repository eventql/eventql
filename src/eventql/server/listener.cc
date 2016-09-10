/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ctime>
#include <sys/time.h>
#include <sys/types.h>
#include <thread>
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif
#include "eventql/util/http/httpserverconnection.h"
#include "eventql/eventql.h"
#include "eventql/server/listener.h"
#include "eventql/server/session.h"
#include "eventql/db/database.h"
#include "eventql/util/return_code.h"
#include "eventql/util/logging.h"
#include "eventql/transport/native/connection_tcp.h"

namespace eventql {

Listener::Listener(
    Database* database) :
    database_(database),
    io_timeout_(
        std::max(
            database->getConfig()->getInt("server.s2s_io_timeout").get(),
            database->getConfig()->getInt("server.c2s_io_timeout").get())),
    running_(true),
    ssock_(-1),
    http_transport_(database),
    native_server_(database) {}

ReturnCode Listener::bind(int listen_port) {
  ssock_ = ::socket(AF_INET, SOCK_STREAM, 0);
  if (ssock_ == 0) {
    return ReturnCode::error(
        "IOERR",
        "create socket() failed: %s",
        strerror(errno));
  }

  int opt = 1;
  if (::setsockopt(ssock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    return ReturnCode::error(
        "IOERR",
        "setsockopt(SO_REUSEADDR) failed: %s",
        strerror(errno));
  }

  int flags = ::fcntl(ssock_, F_GETFL, 0);
  flags = flags | O_NONBLOCK;

  if (::fcntl(ssock_, F_SETFL, flags) != 0) {
    return ReturnCode::error(
        "IOERR",
        "fcntl(F_SETFL, O_NONBLOCK) failed: %s",
        strerror(errno));
  }

  struct sockaddr_in addr;
  memset((char *) &addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(listen_port);
  if (::bind(ssock_, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    return ReturnCode::error(
        "IOERR",
        "bind() failed: %s",
        strerror(errno));
  }

  if (::listen(ssock_, 1024) == -1) {
    return ReturnCode::error(
        "IOERR",
        "listen() failed: %s",
        strerror(errno));
  }

  logNotice("eventql", "Listening on port $0", listen_port);
  return ReturnCode::success();
}

void Listener::run() {
  http_transport_.startIOThread();

  while (running_.load()) {
    fd_set op_read, op_write, op_error;
    FD_ZERO(&op_read);
    FD_ZERO(&op_write);
    FD_ZERO(&op_error);

    int max_fd = ssock_;
    FD_SET(ssock_, &op_read);

    auto now = MonotonicClock::now();
    while (!connections_.empty()) {
      if (connections_.front().accepted_at + io_timeout_ > now) {
        break;
      }

      logError(
          "eventql",
          "timeout while reading first byte, closing connection");

      close(connections_.front().fd);
      connections_.pop_front();
    }

    uint64_t timeout = 0;
    if (!connections_.empty()) {
      timeout = (connections_.front().accepted_at + io_timeout_) - now;
    }

    for (const auto& c : connections_) {
      if (c.fd > max_fd) {
        max_fd = c.fd;
      }

      FD_SET(c.fd, &op_read);
    }

    int res;
    if (timeout) {
      struct timeval tv;
      tv.tv_sec = timeout / kMicrosPerSecond;
      tv.tv_usec = timeout % kMicrosPerSecond;
      res = select(max_fd + 1, &op_read, &op_write, &op_error, &tv);
    } else {
      res = select(max_fd + 1, &op_read, &op_write, &op_error, NULL);
    }

    switch (res) {
      case 0:
        continue;
      case -1:
        if (errno == EINTR) {
          continue;
        }

        logError("eventql", "select() failed");
        goto exit;
    }

    if (FD_ISSET(ssock_, &op_read)) {
      int conn_fd = ::accept(ssock_, NULL, NULL);
      if (conn_fd < 0) {
        logError("eventql", "accept() failed");
        continue;
      }

      int flags = ::fcntl(conn_fd, F_GETFL, 0);
      flags = flags | O_NONBLOCK;

      if (::fcntl(conn_fd, F_SETFL, flags) != 0) {
        logError(
            "eventql",
            "fcntl(F_SETFL, O_NONBLOCK) failed, closing connection: $0",
            strerror(errno));

        close(conn_fd);
        continue;
      }

      EstablishingConnection c;
      c.fd = conn_fd;
      c.accepted_at = MonotonicClock::now();
      connections_.emplace_back(c);
    }

    auto iter = connections_.begin();
    while (iter != connections_.end()) {
      if (FD_ISSET(iter->fd, &op_read)) {
        open(iter->fd);
        iter = connections_.erase(iter);
      } else {
        iter++;
      }
    }
  }

exit:
  http_transport_.stopIOThread();
  return;
}

void Listener::shutdown() {
}

void Listener::open(int fd) {
  char first_byte;
  auto res = ::read(fd, &first_byte, 1);
  if (res != 1) {
    logError(
        "eventql",
        "error while reading first byte, closing connection: $0",
        strerror(errno));

    close(fd);
    return;
  }

  int flags = ::fcntl(fd, F_GETFL, 0);
  flags = flags & ~O_NONBLOCK;

  if (::fcntl(fd, F_SETFL, flags) != 0) {
    logError(
        "eventql",
        "fcntl(F_SETFL, O_NONBLOCK) failed, closing connection: $0",
        strerror(errno));

    close(fd);
    return;
  }

  switch (first_byte) {

    // native
   case '^': {
      native_server_.startConnection(
          std::unique_ptr<native_transport::NativeConnection>(
              new native_transport::TCPConnection(
                  fd,
                  io_timeout_,
                  std::string(&first_byte, 1))));
      break;
    }

    // http
    default:
      http_transport_.handleConnection(fd, std::string(&first_byte, 1));
      break;

  }
}

} // namespace eventql
