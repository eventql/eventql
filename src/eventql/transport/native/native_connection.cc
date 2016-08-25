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
#include "eventql/transport/native/native_connection.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

namespace eventql {

NativeConnection::NativeConnection(int fd) : fd_(fd) {}

NativeConnection::~NativeConnection() { 
  close();
}

ReturnCode NativeConnection::read(
    char* data,
    size_t len,
    uint64_t timeout_us) {
  if (fd_ < 0) {
    return ReturnCode::error("EIO", "connection closed");
  }

  size_t pos = 0;
  while (pos < len) {
    int read_rc = ::read(fd_, data + pos, len - pos);
    switch (read_rc) {
      case 0:
        close();
        return ReturnCode::error("EIO", "connection unexpectedly closed");
      case -1:
        if (errno == EAGAIN || errno == EINTR) {
          break;
        } else {
          close();
          return ReturnCode::error("EIO", strerror(errno));
        }
      default:
        pos += read_rc;
        break;
    }

    struct pollfd p;
    p.fd = fd_;
    p.events = POLLIN;

    int poll_rc = poll(&p, 1, timeout_us / 1000);
    switch (poll_rc) {
      case 0:
        close();
        return ReturnCode::error("EIO", "operation timed out");
      case -1:
        if (errno == EAGAIN || errno == EINTR) {
          break;
        } else {
          close();
          return ReturnCode::error("EIO", strerror(errno));
        }
    }
  }

  ReturnCode::success();
}

ReturnCode NativeConnection::recvFrame(
    uint16_t* opcode,
    std::string* payload,
    uint16_t* flags /* = nullptr */) {
  ReturnCode::success();
}

void NativeConnection::close() {
  if (fd_ < 0) {
    return;
  }

  ::close(fd_);
  fd_ = -1;
}

} // namespace eventql

