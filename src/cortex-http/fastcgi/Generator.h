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
#include <cortex-http/HeaderFieldList.h>
#include <cortex-http/HttpInfo.h>
#include <cortex-http/HttpRequestInfo.h>
#include <cortex-http/HttpResponseInfo.h>
#include <cortex-http/fastcgi/bits.h>
#include <cortex-base/io/FileRef.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/sysconfig.h>

namespace cortex {

class EndPointWriter;

namespace http {
namespace fastcgi {

/**
 * FastCGI Request/Response stream generator.
 *
 * Generates the binary stream for an HTTP request or response.
 */
class CORTEX_HTTP_API Generator {
 public:
  /**
   * Generates an HTTP request or HTTP response for given FastCGI request-ID.
   *
   * @param requestId FastCGI's requestId that is associated for the generated message.
   * @param dateGenerator used for creating the Date message header.
   * @param writer endpoint writer to write the binary stream to.
   */
  Generator(
      int requestId,
      EndPointWriter* writer);

  void generateRequest(const HttpRequestInfo& info);
  void generateRequest(const HttpRequestInfo& info, Buffer&& chunk);
  void generateRequest(const HttpRequestInfo& info, const BufferRef& chunk);
  void generateResponse(const HttpResponseInfo& info);
  void generateResponse(const HttpResponseInfo& info, const BufferRef& chunk);
  void generateResponse(const HttpResponseInfo& info, Buffer&& chunk);
  void generateResponse(const HttpResponseInfo& info, FileRef&& chunk);
  void generateBody(Buffer&& chunk);
  void generateBody(const BufferRef& chunk);
  void generateBody(FileRef&& chunk);
  void generateEnd();

  void flushBuffer();

  size_t bytesTransmitted() const noexcept { return bytesTransmitted_; }

 private:
  void write(Type type, int requestId, const char* buf, size_t len);

 private:
  enum Mode { Nothing, GenerateRequest, GenerateResponse } mode_;
  int requestId_;
  size_t bytesTransmitted_;
  Buffer buffer_;
  EndPointWriter* writer_;
};

} // namespace fastcgi
} // namespace http
} // namespace cortex
