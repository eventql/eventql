// This file is part of the "libstx" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libstx is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
#pragma once
#include <stx/stdtypes.h>
#include <stx/buffer.h>
#include <pcre.h>
#include <string>
#include <vector>

namespace stx {

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

  bool match(const Buffer& subject, Result* result = nullptr) const;
  bool match(const String& subject, Result* result = nullptr) const;
  bool match(const char* buffer, size_t size, Result* result = nullptr) const;
  bool match(const char* cstring, Result* result = nullptr) const;

  /**
   * Returns the index of a named capture in the regex or size_t(-1) if there
   * is no such named capture group in the regular expressin
   */
  size_t getNamedCaptureIndex(const String& name);

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
  pcre* pcre_handle_;
};

}  // namespace stx

namespace std {
inline std::ostream& operator<<(std::ostream& os, const stx::RegExp& re) {
  os << re.pattern();
  return os;
}
}

