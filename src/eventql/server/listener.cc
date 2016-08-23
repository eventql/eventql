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
#include "eventql/eventql.h"
#include "eventql/server/listener.h"
#include "eventql/util/return_code.h"
#include "eventql/util/logging.h"

namespace eventql {

Listener::Listener() : ssock_(-1) {}

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
  ev_.runOnReadable(std::bind(&Listener::accept, this), ssock_);
  return ReturnCode::success();
}

void Listener::start() {
  ev_.run();
}

void Listener::stop() {
  ev_.shutdown();
}

void Listener::accept() {
  int conn_fd = ::accept(ssock_, NULL, NULL);
  if (conn_fd < 0) {
    logError("eventql", "accept() failed");
  }

  int flags = ::fcntl(conn_fd, F_GETFL, 0);
  flags = flags | O_NONBLOCK;

  if (::fcntl(conn_fd, F_SETFL, flags) != 0) {
    logError(
        "eventql",
        "fcntl(F_SETFL, O_NONBLOCK) failed, closing connection: $0",
        strerror(errno));

    close(conn_fd);
    return;
  }

  ev_.runOnReadable(std::bind(&Listener::open, this, conn_fd), conn_fd);
  ev_.runOnReadable(std::bind(&Listener::accept, this), ssock_);
}

void Listener::open(int fd) {
  char first_byte;
  auto res = ::read(fd, &first_byte, 1);
  if (res != 1) {
    if (errno == EWOULDBLOCK) {
      ev_.runOnReadable(std::bind(&Listener::open, this, fd), fd);
      return;
    } else {
      logError(
          "eventql",
          "error while reading first byte, closing connection: $0",
          strerror(errno));

      close(fd);
      return;
    }
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

  iputs("gotconn: $0 -> $1", fd, std::string(&first_byte, 1));
}

} // namespace eventql
