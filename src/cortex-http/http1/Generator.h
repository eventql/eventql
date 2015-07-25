// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <cortex-http/HeaderFieldList.h>
#include <cortex-base/io/FileRef.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/sysconfig.h>
#include <memory>

namespace cortex {

class EndPointWriter;

namespace http {

class HttpDateGenerator;
class HttpInfo;
class HttpRequestInfo;
class HttpResponseInfo;

namespace http1 {

/**
 * Generates HTTP/1 messages.
 *
 * Implements the HTTP/1.1 transport layer syntax to generate
 * RFC 7230 conform messages.
 */
class CORTEX_HTTP_API Generator {
  enum class State {
    None,
    WritingBody,
    Closed
  };

 public:
  explicit Generator(EndPointWriter* output);

  /** resets any runtime state. */
  void recycle();

  /**
   * Generates an HTTP request message.
   *
   * @param info HTTP request message info.
   * @param chunk HTTP message body chunk.
   */
  void generateRequest(const HttpRequestInfo& info, Buffer&& chunk);
  void generateRequest(const HttpRequestInfo& info, const BufferRef& chunk);

  /**
   * Generates an HTTP response message.
   *
   * @param info HTTP response message info.
   * @param chunk HTTP message body chunk.
   */
  void generateResponse(const HttpResponseInfo& info, const BufferRef& chunk);
  void generateResponse(const HttpResponseInfo& info, Buffer&& chunk);
  void generateResponse(const HttpResponseInfo& info, FileRef&& chunk);

  /**
   * Generates an HTTP message body chunk.
   *
   * @param chunk HTTP message body chunk.
   */
  void generateBody(Buffer&& chunk);

  /**
   * Generates an HTTP message body chunk.
   *
   * @param chunk HTTP message body chunk.
   */
  void generateBody(const BufferRef& chunk);

  /**
   * Generates an HTTP message body chunk.
   *
   * @param chunk HTTP message body chunk, represented as a file.
   */
  void generateBody(FileRef&& chunk);

  /**
   * Generates possibly pending bytes & trailers to complete the HTTP message.
   */
  void generateTrailer(const HeaderFieldList& trailers);

  /**
   * Retrieves the number of bytes pending for the content.
   */
  size_t pendingContentLength() const CORTEX_NOEXCEPT { return contentLength_; }

  /**
   * Retrieves boolean indicating whether chunked response is generated.
   */
  bool isChunked() const CORTEX_NOEXCEPT { return chunked_; }

  size_t bytesTransmitted() const noexcept { return bytesTransmitted_; }

 private:
  void generateRequestLine(const HttpRequestInfo& info);
  void generateResponseLine(const HttpResponseInfo& info);
  void generateHeaders(const HttpInfo& info);
  void generateResponseInfo(const HttpResponseInfo& info);
  void flushBuffer();

 private:
  size_t bytesTransmitted_;
  size_t contentLength_;
  bool chunked_;
  Buffer buffer_;
  EndPointWriter* writer_;
};

}  // namespace http1
}  // namespace http
}  // namespace cortex
