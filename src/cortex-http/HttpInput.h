// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <cortex-base/sysconfig.h>

namespace cortex {

class Buffer;
class BufferRef;

namespace http {

class HttpInputListener;

/**
 * Abstract HTTP message body consumer API.
 */
class CORTEX_HTTP_API HttpInput {
 public:
  HttpInput();
  virtual ~HttpInput();

  /**
   * Reads some data into the end of @p result.
   */
  virtual int read(Buffer* result) = 0;

  /**
   * Reads a single line and stores it in @p result without trailing linefeed.
   */
  virtual size_t readLine(Buffer* result) = 0;

  /**
   * Tests whether read or readLine will succeed, and thus, data is readable.
   */
  virtual bool empty() const noexcept = 0;

  /**
   * Registers a callback interface to get notified when input data is
   * available.
   *
   * @param listener the callback interface to invoke when input data is
   *                 available.
   */
  void setListener(HttpInputListener* listener);

  /**
   * Retrieves the HttpInputListener that is being associated with this input.
   */
  HttpInputListener* listener() const CORTEX_NOEXCEPT { return listener_; }

  /**
   * Internally invoked to pass some input chunk to this layer.
   *
   * @param chunk the raw request body data chunk.
   *
   * @internal
   */
  virtual void onContent(const BufferRef& chunk) = 0;

  /**
   * Internally invoked by HttpRequest::recycle() to recycle the object.
   * @internal
   */
  virtual void recycle() = 0;

 private:
  HttpInputListener* listener_;
};

}  // namespace http
}  // namespace cortex
