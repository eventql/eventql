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
#include "eventql/transport/native/connection_tcp.h"
#include "eventql/util/inspect.h"
#include "eventql/util/util/binarymessagewriter.h"
#include "eventql/util/logging.h"
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
#include <netinet/tcp.h>

namespace eventql {
namespace native_transport {

TCPConnection::TCPConnection(
    int fd,
    uint64_t io_timeout,
    const std::string& prelude_bytes /* = "" */) :
    fd_(fd),
    read_buf_(prelude_bytes),
    io_timeout_(io_timeout) {
  logTrace("eventql", "Opening new tcp connection; fd=$1", fd);

  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

  size_t nodelay = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

  struct sockaddr_storage saddr;
  socklen_t slen = sizeof(saddr);
  if (getpeername(fd, (struct sockaddr*) &saddr, &slen) == 0) {
    char ipstr[INET6_ADDRSTRLEN];
    switch (saddr.ss_family) {
      case AF_INET: {
        struct sockaddr_in* s = (struct sockaddr_in*) &saddr;
        if (inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr))) {
          remote_host_ = std::string(ipstr);
        }
        break;
      }
      case AF_INET6: {
        struct sockaddr_in6* s = (struct sockaddr_in6*) &saddr;
        if (inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof(ipstr))) {
          remote_host_ = std::string(ipstr);
        }
        break;
      }
    }
  }
}

TCPConnection::~TCPConnection() {
  close();
}

ReturnCode TCPConnection::read(
    char* data,
    size_t len,
    uint64_t timeout_us) {
  size_t pos = 0;
  if (!read_buf_.empty()) {
    pos = std::min(len, read_buf_.size());
    memcpy(data, read_buf_.data(), pos);
    read_buf_ = read_buf_.substr(pos);
  }

  if (fd_ < 0) {
    return ReturnCode::error("EIO", "connection closed");
  }

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

    if (pos == len) {
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

  return ReturnCode::success();
}

ReturnCode TCPConnection::recvFrame(
    uint16_t* opcode,
    uint16_t* recvflags,
    std::string* payload,
    uint64_t timeout_us) {
  char header[8];
  auto rc = read(header, sizeof(header), timeout_us);
  if (!rc.isSuccess()) {
    return rc;
  }

  *opcode = ntohs(*((uint16_t*) &header[0]));
  uint16_t flags = ntohs(*((uint16_t*) &header[2]));
  uint32_t payload_len = ntohl(*((uint32_t*) &header[4]));
  if (recvflags) {
    *recvflags = flags;
  }

  if (payload_len > kMaxFrameSize) {
    close();
    return ReturnCode::error("EIO", "received invalid frame header");
  }

  payload->resize(payload_len);
  return read(&(*payload)[0], payload_len, io_timeout_);
}

ReturnCode TCPConnection::sendFrame(
    uint16_t opcode,
    uint16_t flags,
    const void* payload,
    size_t payload_len) {
  auto rc = sendFrameAsync(opcode, flags, payload, payload_len);
  if (!rc.isSuccess()) {
    return rc;
  }

  return flushOutbox(true, io_timeout_);
}

ReturnCode TCPConnection::sendFrameAsync(
    uint16_t opcode,
    uint16_t flags,
    const void* data,
    size_t len) {
  writeFrameHeaderAsync(opcode, len, flags);
  writeAsync(data, len);
  return flushOutbox(false, 0);
}

ReturnCode TCPConnection::flushOutbox(
    bool block,
    uint64_t timeout_us /* = 0 */) {
  if (fd_ < 0) {
    return ReturnCode::error("EIO", "connection closed");
  }

  if (block && !timeout_us) {
    timeout_us = io_timeout_;
  }

  while (!write_buf_.empty()) {
    int write_rc = ::write(fd_, write_buf_.data(), write_buf_.size());
    switch (write_rc) {
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
        write_buf_ = write_buf_.substr(write_rc);
        break;
    }

    if (write_buf_.empty() || !block) {
      break;
    }

    struct pollfd p;
    p.fd = fd_;
    p.events = POLLOUT;

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

  return ReturnCode::success();
}

void TCPConnection::writeFrameHeaderAsync(
    uint16_t opcode,
    size_t len,
    uint16_t flags /* = 0 */) {
  uint16_t opcode_n = htons(opcode);
  uint16_t flags_n = htons(flags);
  uint32_t payload_len_n = htonl(len);

  char header[8];
  memcpy(&header[0], (const char*) &opcode_n, 2);
  memcpy(&header[2], (const char*) &flags_n, 2);
  memcpy(&header[4], (const char*) &payload_len_n, 4);
  writeAsync(header, sizeof(header));
}

void TCPConnection::writeAsync(const void* data, size_t len) {
  write_buf_ += std::string((const char*) data, len);
}

bool TCPConnection::isOutboxEmpty() const {
  return write_buf_.empty();
}

void TCPConnection::close() {
  if (fd_ < 0) {
    return;
  }

  ::close(fd_);
  fd_ = -1;
}

void TCPConnection::setIOTimeout(uint64_t timeout_us) {
  io_timeout_ = timeout_us;
}

std::string TCPConnection::getRemoteHost() const {
  return remote_host_;
}

} // namespace native_connection
} // namespace eventql

