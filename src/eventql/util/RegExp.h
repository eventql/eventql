/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Christian Parpart <trapni@dawanda.com>
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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/buffer.h>
//#include <eventql/sysconfig.h>
#include <string>
#include <vector>

#ifdef HAVE_PCRE
#include <pcre.h>
#else
#include <regex>
#endif

class RegExp {
public:
  typedef std::vector<std::pair<const char*, size_t>> Result;

  explicit RegExp(const std::string& pattern);
  RegExp();
  RegExp(const RegExp& other);
  ~RegExp();

  RegExp(RegExp&& v);
  RegExp& operator=(const RegExp& other) = delete;
  RegExp& operator=(RegExp&& other);

  bool match(const Buffer& subject) const;
  bool match(const String& subject) const;
  bool match(const char* buffer, size_t size) const;
  bool match(const char* cstring) const;

#ifdef HAVE_PCRE
  bool match(const Buffer& subject, Result* result) const;
  bool match(const String& subject, Result* result) const;
  bool match(const char* buffer, size_t size, Result* result) const;
  bool match(const char* cstring, Result* result) const;

  /**
   * Returns the index of a named capture in the regex or size_t(-1) if there
   * is no such named capture group in the regular expressin
   */
  size_t getNamedCaptureIndex(const String& name);
#endif

  const std::string& pattern() const { return pattern_; }
  const char* c_str() const;

  operator const std::string&() const { return pattern_; }

  friend bool operator==(const RegExp& a, const RegExp& b) {
    return a.pattern() == b.pattern();
  }

  friend bool operator!=(const RegExp& a, const RegExp& b) {
    return a.pattern() != b.pattern();
  }

  friend bool operator<=(const RegExp& a, const RegExp& b) {
    return a.pattern() <= b.pattern();
  }

  friend bool operator>=(const RegExp& a, const RegExp& b) {
    return a.pattern() >= b.pattern();
  }

  friend bool operator<(const RegExp& a, const RegExp& b) {
    return a.pattern() < b.pattern();
  }

  friend bool operator>(const RegExp& a, const RegExp& b) {
    return a.pattern() > b.pattern();
  }

private:
  std::string pattern_;
#ifdef HAVE_PCRE
  pcre* pcre_handle_;
#else
  std::regex re_;
#endif
};


namespace std {
inline std::ostream& operator<<(std::ostream& os, const RegExp& re) {
  os << re.pattern();
  return os;
}
}

