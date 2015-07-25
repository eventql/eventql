// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <cortex-base/sysconfig.h>
#include <cortex-http/HttpInfo.h>
#include <string>

namespace cortex {
namespace http {

/**
 * HTTP Request Message Info.
 */
class CORTEX_HTTP_API HttpRequestInfo : public HttpInfo {
 public:
  HttpRequestInfo();
  HttpRequestInfo(HttpVersion version, const std::string& method,
                  const std::string& entity, size_t contentLength,
                  const HeaderFieldList& headers);

  const std::string& method() const CORTEX_NOEXCEPT { return method_; }
  const std::string& entity() const CORTEX_NOEXCEPT { return entity_; }

 private:
  std::string method_;
  std::string entity_;
};

inline HttpRequestInfo::HttpRequestInfo()
    : HttpRequestInfo(HttpVersion::UNKNOWN, "", "", 0, {}) {
}

inline HttpRequestInfo::HttpRequestInfo(HttpVersion version,
                                        const std::string& method,
                                        const std::string& entity,
                                        size_t contentLength,
                                        const HeaderFieldList& headers)
    : HttpInfo(version, contentLength, headers, {}),
      method_(method),
      entity_(entity) {
}

}  // namespace http
}  // namespace cortex
