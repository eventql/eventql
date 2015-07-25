// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <cortex-base/sysconfig.h>
#include <unordered_map>
#include <string>

namespace cortex {
namespace http {

class HttpRequest;
class HttpResponse;

/**
 * HTTP response output compression.
 */
class CORTEX_HTTP_API HttpOutputCompressor {
 public:
  HttpOutputCompressor();
  ~HttpOutputCompressor();

  void addMimeType(const std::string& value);
  bool containsMimeType(const std::string& value) const;

  void setMinSize(size_t value);
  size_t minSize() const CORTEX_NOEXCEPT { return minSize_; }

  void setMaxSize(size_t value);
  size_t maxSize() const CORTEX_NOEXCEPT { return maxSize_; }

  void setCompressionLevel(int value) { level_ = value; }
  int compressionLevel() const CORTEX_NOEXCEPT { return level_; }

  /**
   * Injects a preCommit handler to automatically add output compression.
   */
  void inject(HttpRequest* request, HttpResponse* response);

  /**
   * Adds output compression to @p response if @p request set to accept any.
   */
  void postProcess(HttpRequest* request, HttpResponse* response);

 private:
  size_t minSize_;
  size_t maxSize_;
  int level_;
  std::unordered_map<std::string, int> contentTypes_;
};

} // namespace http
} // namespace cortex
