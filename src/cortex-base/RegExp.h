// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef sw_flow_RegExp_h
#define sw_flow_RegExp_h

#include <cortex-base/Api.h>
#include <pcre.h>
#include <string>
#include <vector>

namespace cortex {

class BufferRef;

class CORTEX_API RegExp {
 private:
  std::string pattern_;
  pcre* re_;

 public:
  typedef std::vector<std::pair<const char*, size_t>> Result;

 public:
  explicit RegExp(const std::string& pattern);
  RegExp();
  RegExp(const RegExp& v);
  ~RegExp();

  RegExp(RegExp&& v);
  RegExp& operator=(RegExp&& v);

  bool match(const char* buffer, size_t size, Result* result = nullptr) const;
  bool match(const BufferRef& buffer, Result* result = nullptr) const;
  bool match(const char* cstring, Result* result = nullptr) const;

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
};

class CORTEX_API RegExpContext {
 public:
  RegExpContext();
  virtual ~RegExpContext();

  RegExp::Result* regexMatch() {
    if (!regexMatch_) regexMatch_ = new RegExp::Result();

    return regexMatch_;
  }

 private:
  RegExp::Result* regexMatch_;
};

}  // namespace cortex

namespace std {
inline std::ostream& operator<<(std::ostream& os, const cortex::RegExp& re) {
  os << re.pattern();
  return os;
}
}

#endif
