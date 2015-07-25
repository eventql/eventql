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
#include <cortex-base/DateTime.h>
#include <vector>
#include <string>

namespace cortex {
namespace http {

class Cookies {
public:
  typedef std::vector<std::pair<std::string, std::string>> CookieList;

  static bool getCookie(
      const CookieList& cookies,
      const std::string& key,
      std::string* dst);

  static CookieList parseCookieHeader(const std::string& header_str);

  static std::string makeCookie(
      const std::string& key,
      const std::string& value,
      const DateTime& expire = DateTime::epoch(),
      const std::string& path = "",
      const std::string& domain = "",
      bool secure = false,
      bool httpOnly = false);

  CORTEX_DEPRECATED static std::string mkCookie(
      const std::string& key,
      const std::string& value,
      const DateTime& expire = DateTime::epoch(),
      const std::string& path = "",
      const std::string& domain = "",
      bool secure = false,
      bool httpOnly = false) {
    return makeCookie(key, value, expire, path, domain, secure, httpOnly);
  }
};

} // namespace http
} // namespace cortex
