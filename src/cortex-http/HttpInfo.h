// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <cortex-base/sysconfig.h>
#include <cortex-http/HttpVersion.h>
#include <cortex-http/HeaderFieldList.h>
#include <string>

namespace cortex {
namespace http {

/**
 * Base HTTP Message Info.
 *
 * @see HttpRequestInfo
 * @see HttpResponseInfo
 */
class CORTEX_HTTP_API HttpInfo {
 public:
  HttpInfo(HttpVersion version, size_t contentLength,
           const HeaderFieldList& headers,
           const HeaderFieldList& trailers);

  /** Retrieves the HTTP message version. */
  HttpVersion version() const CORTEX_NOEXCEPT { return version_; }

  /** Retrieves the HTTP response headers. */
  const HeaderFieldList& headers() const CORTEX_NOEXCEPT { return headers_; }

  /** Retrieves the HTTP response headers. */
  HeaderFieldList& headers() CORTEX_NOEXCEPT { return headers_; }

  void setContentLength(size_t size);
  size_t contentLength() const CORTEX_NOEXCEPT { return contentLength_; }

  bool hasContentLength() const CORTEX_NOEXCEPT {
    return contentLength_ != static_cast<size_t>(-1);
  }

  /** Tests whether HTTP message will send trailers. */
  bool hasTrailers() const CORTEX_NOEXCEPT { return !trailers_.empty(); }

  /** Retrieves the HTTP response trailers. */
  const HeaderFieldList& trailers() const CORTEX_NOEXCEPT { return trailers_; }

  void setTrailers(const HeaderFieldList& list) { trailers_ = list; }

 protected:
  HttpVersion version_;
  size_t contentLength_;
  HeaderFieldList headers_;
  HeaderFieldList trailers_;
};

inline HttpInfo::HttpInfo(HttpVersion version, size_t contentLength,
                          const HeaderFieldList& headers,
                          const HeaderFieldList& trailers)
    : version_(version),
      contentLength_(contentLength),
      headers_(headers),
      trailers_(trailers) {
  //.
}

}  // namespace http
}  // namespace cortex
