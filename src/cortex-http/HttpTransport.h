// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/HeaderFieldList.h>
#include <cortex-http/HttpStatus.h>
#include <cortex-base/net/Connection.h>
#include <cortex-base/CompletionHandler.h>
#include <cortex-base/Buffer.h>
#include <memory>

namespace cortex {

class FileRef;

namespace http {

class HttpResponseInfo;

/**
 * HTTP transport layer interface.
 *
 * Implemements the wire transport protocol for HTTP messages without
 * any semantics.
 *
 * For HTTP/1 for example it is <b>RFC 7230</b>.
 */
class CORTEX_HTTP_API HttpTransport {
 public:
  virtual ~HttpTransport();

  /**
   * Cancels communication completely.
   */
  virtual void abort() = 0;

  /**
   * Invoked when the currently generated response has been fully transmitted.
   */
  virtual void completed() = 0;

  /**
   * Initiates sending a response to the client.
   *
   * @param responseInfo HTTP response meta informations.
   * @param chunk response body data chunk.
   * @param onComplete callback invoked when sending chunk is succeed/failed.
   *
   * @note You must ensure the data chunk is available until sending completed!
   */
  virtual void send(HttpResponseInfo&& responseInfo, const BufferRef& chunk,
                    CompletionHandler onComplete) = 0;

  /**
   * Initiates sending a response to the client.
   *
   * @param responseInfo HTTP response meta informations.
   * @param chunk response body data chunk.
   * @param onComplete callback invoked when sending chunk is succeed/failed.
   */
  virtual void send(HttpResponseInfo&& responseInfo, Buffer&& chunk,
                    CompletionHandler onComplete) = 0;

  /**
   * Initiates sending a response to the client.
   *
   * @param responseInfo HTTP response meta informations.
   * @param chunk response body chunk represented as a file.
   * @param onComplete callback invoked when sending chunk is succeed/failed.
   */
  virtual void send(HttpResponseInfo&& responseInfo, FileRef&& chunk,
                    CompletionHandler onComplete) = 0;

  /**
   * Transfers this data chunk to the output stream.
   *
   * @param chunk response body chunk
   * @param onComplete callback invoked when sending chunk is succeed/failed.
   */
  virtual void send(Buffer&& chunk, CompletionHandler onComplete) = 0;

  /**
   * Transfers this file data chunk to the output stream.
   *
   * @param chunk response body chunk represented as a file.
   * @param onComplete callback invoked when sending chunk is succeed/failed.
   */
  virtual void send(FileRef&& chunk, CompletionHandler onComplete) = 0;

  /**
   * Transfers this data chunk to the output stream.
   *
   * @param chunk response body chunk
   * @param onComplete callback invoked when sending chunk is succeed/failed.
   */
  virtual void send(const BufferRef& chunk, CompletionHandler onComplete) = 0;
};

}  // namespace http
}  // namespace cortex
