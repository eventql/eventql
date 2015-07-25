// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <string>
#include <vector>
#include <utility>

namespace cortex {

/**
 * Represents a parsed URI.
 */
class CORTEX_API Uri {
public:
  typedef std::vector<std::pair<std::string, std::string>> ParamList;

  static std::string encode(const std::string& str);
  static std::string decode(const std::string& str);

  Uri();
  explicit Uri(const std::string& uri);

  void parse(const std::string& uri);

  const std::string& scheme() const;
  const std::string& userinfo() const;
  const std::string& host() const;
  const unsigned port() const;
  std::string hostAndPort() const;
  const std::string& path() const;
  const std::string& query() const;
  std::string pathAndQuery() const;
  ParamList queryParams() const;
  const std::string& fragment() const;
  std::string toString() const;

  static void parseURI(
      const std::string& uri_str,
      std::string* scheme,
      std::string* userinfo,
      std::string* host,
      unsigned* port,
      std::string* path,
      std::string* query,
      std::string* fragment);

  static void parseQueryString(const std::string& query, ParamList* params);

  static bool getParam(
      const ParamList&,
      const std::string& key,
      std::string* value);

protected:
  std::string scheme_;
  std::string userinfo_;
  std::string host_;
  unsigned port_;
  std::string path_;
  std::string query_;
  std::string fragment_;
};

} // namespace cortex
