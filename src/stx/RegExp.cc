// This file is part of the "libstx" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libstx is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
#include <stx/RegExp.h>
#include <cstring>
#include <pcre.h>

namespace stx {

RegExp::RegExp() : pcre_handle_(nullptr) {}

RegExp::RegExp(const RegExp& other) : pattern_(other.pattern_) {
  // there's no pcpcre_handle_clone() unfortunately ^^
  const char* error_msg = "";
  int error_pos = 0;

  pcre_handle_ = pcre_compile(
      pattern_.c_str(),
      PCRE_CASELESS | PCRE_EXTENDED,
      &error_msg,
      &error_pos,
      0);

  if (!pcre_handle_) {
    RAISEF(kParseError, "Invalid regex: $0", error_msg);
  }
}

RegExp::RegExp(const std::string& pattern) : pattern_(pattern) {
  const char* error_msg = "";
  int error_pos = 0;

  pcre_handle_ = pcre_compile(
      pattern_.c_str(),
      0,
      &error_msg,
      &error_pos,
      0);

  if (!pcre_handle_) {
    RAISEF(kParseError, "Invalid regex: $0", error_msg);
  }
}

RegExp::~RegExp() {
  if (pcre_handle_) {
    pcre_free(pcre_handle_);
  }
}

RegExp::RegExp(
    RegExp&& other) :
    pattern_(std::move(other.pattern_)),
    pcre_handle_(other.pcre_handle_) {
  other.pcre_handle_ = nullptr;
}

RegExp& RegExp::operator=(RegExp&& other) {
  pattern_ = std::move(other.pattern_);

  if (pcre_handle_) {
    pcre_free(pcre_handle_);
  }

  pcre_handle_ = other.pcre_handle_;
  other.pcre_handle_ = nullptr;

  return *this;
}

bool RegExp::match(const char* buffer, size_t size, Result* result) const {
  if (!pcre_handle_) return false;

  const size_t OV_COUNT = 3 * 36;
  int ov[OV_COUNT];

#ifndef NDEBUG
  for (size_t i = 0; i < OV_COUNT; ++i)
    ov[i] = 1337;
#endif

  int rc = pcre_exec(pcre_handle_, nullptr, buffer, size, 0, 0, ov, OV_COUNT);
  if (result) {
    result->clear();
    if (rc > 0) {
      for (size_t i = 0, e = rc * 2; i != e; i += 2) {
        const char* value = buffer + ov[i];
        size_t length = ov[i + 1] - ov[i];
        result->push_back(std::make_pair(value, length));
      }
    } else {
      result->push_back(std::make_pair("", 0));
    }
  }
  return rc > 0;
}

bool RegExp::match(const String& subject, Result* result) const {
  return match(subject.data(), subject.size(), result);
}

bool RegExp::match(const Buffer& subject, Result* result) const {
  return match((char*) subject.data(), subject.size(), result);
}

bool RegExp::match(const char* cstring, Result* result) const {
  return match(cstring, strlen(cstring), result);
}

size_t RegExp::getNamedCaptureIndex(const String& name) {
  auto res = pcre_get_stringnumber(pcre_handle_, name.c_str());

  if (res == PCRE_ERROR_NOSUBSTRING) {
    return size_t(-1);
  } else {
    return res;
  }
}

const char* RegExp::c_str() const {
  return pattern_.c_str();
}

}  // namespace stx
