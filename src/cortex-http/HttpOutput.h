// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <cortex-base/sysconfig.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/CompletionHandler.h>
#include <functional>
#include <memory>

namespace cortex {

class FileRef;
class Filter;

namespace http {

class HttpChannel;

/**
 * Represents the HTTP response body producer API.
 */
class CORTEX_HTTP_API HttpOutput {
 public:
  explicit HttpOutput(HttpChannel* channel);
  virtual ~HttpOutput();

  virtual void recycle();

  /**
   * Adds a custom output-filter.
   *
   * @param filter the output filter to apply to the output body.
   *
   * The filter will not take over ownership. Make sure the filter is
   * available for the whole time the response is generated.
   */
  void addFilter(std::shared_ptr<Filter> filter);

  /**
   * Removes all output-filters.
   */
  void removeAllFilters();

  /**
   * Writes given C-string @p cstr to the client.
   *
   * @param cstr the null-terminated string that is being copied
   *             into the response body.
   * @param completed Callback to invoke after completion.
   *
   * The C string will be copied into the response body.
   */
  virtual void write(const char* cstr, CompletionHandler&& completed = nullptr);

  /**
   * Writes given string @p str to the client.
   *
   * @param str the string chunk to write to the client. The string will be
   *            copied.
   * @param completed Callback to invoke after completion.
   */
  virtual void write(const std::string& str, CompletionHandler&& completed = nullptr);

  /**
   * Writes given buffer.
   *
   * @param data The data chunk to write to the client.
   * @param completed Callback to invoke after completion.
   */
  virtual void write(Buffer&& data, CompletionHandler&& completed = nullptr);

  /**
   * Writes given buffer.
   *
   * @param data The data chunk to write to the client.
   * @param completed Callback to invoke after completion.
   *
   * @note You must ensure the data chunk is available until sending completed!
   */
  virtual void write(const BufferRef& data, CompletionHandler&& completed = nullptr);

  /**
   * Writes the data received from the given file descriptor @p file.
   *
   * @param file file ref handle
   * @param completed Callback to invoke after completion.
   */
  virtual void write(FileRef&& file, CompletionHandler&& completed = nullptr);

  /**
   * Invoked by HttpResponse::completed().
   */
  virtual void completed();

  size_t size() const CORTEX_NOEXCEPT { return size_; }

 private:
  HttpChannel* channel_;
  size_t size_;
};

}  // namespace http
}  // namespace cortex
