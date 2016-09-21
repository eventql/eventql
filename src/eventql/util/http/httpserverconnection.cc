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
#include "eventql/util/exception.h"
#include "eventql/util/inspect.h"
#include "eventql/util/logging.h"
#include "eventql/util/http/httpserverconnection.h"
#include "eventql/util/http/httpgenerator.h"

template <>
std::string inspect(const http::HTTPServerConnection& conn) {
  return StringUtil::format("<HTTPServerConnection $0>", inspect(&conn));
}

namespace http {

HTTPServerConnection::HTTPServerConnection(
    int fd,
    uint64_t io_timeout,
    const std::string& prelude_bytes) :
    fd_(fd),
    io_timeout_(io_timeout),
    prelude_bytes_(prelude_bytes),
    closed_(false) {
  logTrace("http.server", "New HTTP connection: $0", inspect(*this));

  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

  size_t nodelay = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
}

HTTPServerConnection::~HTTPServerConnection() {
  close();
}

ReturnCode HTTPServerConnection::recvRequest(HTTPRequest* request) {
  HTTPParser parser(HTTPParser::PARSE_HTTP_REQUEST);

  parser.onMethod([request] (HTTPMessage::kHTTPMethod method) {
    request->setMethod(method);
  });

  parser.onURI([request] (const char* data, size_t size) {
    request->setURI(std::string(data, size));
  });

  parser.onVersion([request] (const char* data, size_t size) {
    request->setVersion(std::string(data, size));
  });

  parser.onHeader([request] (
      const char* key,
      size_t key_size,
      const char* val,
      size_t val_size) {
    request->addHeader(
        std::string(key, key_size),
        std::string(val, val_size));
  });

  parser.onBodyChunk([request] (
      const char* data,
      size_t size) {
    request->appendBody(data, size);
  });


  if (!prelude_bytes_.empty()) {
    try {
      parser.parse(prelude_bytes_.data(), prelude_bytes_.size());
    } catch (const std::exception& e) {
      return ReturnCode::error("EPARSE", e.what());
    }

    prelude_bytes_.clear();
  }

  std::string read_buf;
  read_buf.resize(4096);

  while (parser.state() != HTTPParser::S_DONE) {
    int read_rc = ::read(fd_, &read_buf[0], read_buf.size());
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
        try {
          parser.parse(read_buf.data(), read_rc);
        } catch (const std::exception& e) {
          return ReturnCode::error("EPARSE", e.what());
        }
        break;
    }

    if (parser.state() == HTTPParser::S_DONE) {
      break;
    }

    struct pollfd p;
    p.fd = fd_;
    p.events = POLLIN;

    int poll_rc = poll(&p, 1, io_timeout_ / 1000);
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

ReturnCode HTTPServerConnection::sendResponse(const HTTPResponse& res) {
  auto rc = sendResponseHeaders(res);
  if (!rc.isSuccess()) {
    return rc;
  }

  return sendResponseBodyChunk(
      (const char*) res.body().data(),
      res.body().size());
}

ReturnCode HTTPServerConnection::sendResponseHeaders(const HTTPResponse& res) {
  Buffer write_buf;
  BufferOutputStream os(&write_buf);
  HTTPGenerator::generateHeaders(res, &os);

  return write((const char*) write_buf.data(), write_buf.size());
}

ReturnCode HTTPServerConnection::sendResponseBodyChunk(
    const char* data,
    size_t size) {
  return write(data, size);
}

ReturnCode HTTPServerConnection::write(const char* data, size_t size) {
  size_t pos = 0;
  while (pos < size) {
    int write_rc = ::write(fd_, data + pos, size - pos);
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
        pos += write_rc;
        break;
    }

    if (pos == size) {
      break;
    }

    struct pollfd p;
    p.fd = fd_;
    p.events = POLLOUT;
    int poll_rc = poll(&p, 1, io_timeout_ / 1000);
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

bool HTTPServerConnection::isClosed() const {
  return closed_;
}

void HTTPServerConnection::close() {
  if (closed_) {
    return;
  }

  logTrace("http.server", "HTTP connection close: $0", inspect(*this));

  ::close(fd_);
  closed_ = true;
}

} // namespace http

