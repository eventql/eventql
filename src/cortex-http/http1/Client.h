// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
#pragma once

#include <cortex-http/Api.h>
#include <cortex-http/http1/Generator.h>
#include <cortex-http/http1/Parser.h>
#include <cortex-http/HttpVersion.h>
#include <cortex-http/HttpStatus.h>
#include <cortex-http/HeaderFieldList.h>
#include <cortex-base/net/IPAddress.h>
#include <cortex-base/net/EndPointWriter.h>
#include <cortex-base/net/EndPoint.h>
#include <cortex-base/executor/Scheduler.h>
#include <cortex-base/Buffer.h>
#include <string>

namespace cortex {
namespace http {

/**
 * HTTP client response object.
 */
class ClientResponse { // {{{
 public:
  ClientResponse();
  ~ClientResponse();

  HttpVersion version() const noexcept { return version_; }
  HttpStatus status() const noexcept { return status_; }
  const HeaderFieldList& headers() const noexcept { return headers_; }
  const Buffer& body() const noexcept { return body_; }

 private:
  HttpVersion version_;
  HttpStatus status_;
  HeaderFieldList headers_;
  Buffer body_;
}; // }}}

/**
 * HTTP client API.
 *
 * Requirements:
 * - I/O timeouts
 * - sync & async calls.
 * - incremental response receive vs full response object
 */
class CORTEX_HTTP_API Client {
 public:
  Client(HttpVersion version,
         const std::string& method,
         const std::string& entity,
         const HeaderFieldList& headers,
         FileRef&& body);

  Client(HttpVersion version,
         const std::string& method,
         const std::string& entity,
         const HeaderFieldList& headers,
         Buffer&& body);

  Client(HttpVersion version,
         const std::string& method,
         const std::string& entity,
         const HeaderFieldList& headers,
         const BufferRef& body);

  Client(HttpVersion version,
         const std::string& method,
         const std::string& entity,
         const HeaderFieldList& headers);

  void send(const IPAddress& host, int port, HttpListener* response);
  void send(const std::string& unixsockpath, HttpListener* response);

 private:
  Scheduler* scheduler_;
  EndPoint* endpoint_;
  EndPointWriter writer_;
  Generator generator_;
  Parser parser_;
};

}  // namespace http
}  // namespace cortex
