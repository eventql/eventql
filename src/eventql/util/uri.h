/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _STX_BASE_URI_H
#define _STX_BASE_URI_H
#include <string>
#include <vector>
#include <utility>

class URI {
public:
  typedef std::vector<std::pair<std::string, std::string>> ParamList;

  static std::string urlEncode(const std::string& str);
  static std::string urlDecode(const std::string& str);

  URI();
  URI(const std::string& uri_str);
  void parse(const std::string& uri_str);

  const std::string& scheme() const;
  const std::string& userinfo() const;
  const std::string& host() const;
  unsigned port() const;
  std::string hostAndPort() const;
  const std::string& path() const;
  const std::string& query() const;
  std::string pathAndQuery() const;
  ParamList queryParams() const;
  const std::string& fragment() const;
  std::string toString() const;

  void setPath(const std::string& path);

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
  static std::string buildQueryString(const ParamList& params);

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

#endif
