// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/RegExp.h>
#include <xzero/Buffer.h>
#include <cstring>
#include <pcre.h>

namespace xzero {

RegExp::RegExp()
    : pattern_(),
      re_(nullptr) {
}

RegExp::RegExp(const RegExp& v) : pattern_(v.pattern_), re_() {
  // there's no pcre_clone() unfortunately ^^
  const char* errMsg = "";
  int errOfs = 0;

  re_ = pcre_compile(pattern_.c_str(), PCRE_CASELESS | PCRE_EXTENDED, &errMsg,
                     &errOfs, 0);
}

RegExp::RegExp(const std::string& pattern) : pattern_(pattern), re_() {
  const char* errMsg = "";
  int errOfs = 0;

  re_ = pcre_compile(pattern_.c_str(), PCRE_CASELESS | PCRE_EXTENDED, &errMsg,
                     &errOfs, 0);
}

RegExp::~RegExp() {
  pcre_free(re_);
}

RegExp::RegExp(RegExp&& v)
    : pattern_(std::move(v.pattern_)),
      re_(v.re_) {
  v.re_ = nullptr;
}

RegExp& RegExp::operator=(RegExp&& v) {
  pattern_ = std::move(v.pattern_);

  if (re_) {
    pcre_free(re_);
  }
  re_ = v.re_;
  v.re_ = nullptr;

  return *this;
}

bool RegExp::match(const char* buffer, size_t size, Result* result) const {
  if (!re_) return false;

  const size_t OV_COUNT = 3 * 36;
  int ov[OV_COUNT];

#ifndef NDEBUG
  for (size_t i = 0; i < OV_COUNT; ++i)
    ov[i] = 1337;
#endif

  int rc = pcre_exec(re_, nullptr, buffer, size, 0, 0, ov, OV_COUNT);
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

bool RegExp::match(const BufferRef& buffer, Result* result) const {
  return match(buffer.data(), buffer.size(), result);
}

bool RegExp::match(const char* cstring, Result* result) const {
  return match(cstring, strlen(cstring), result);
}

const char* RegExp::c_str() const {
  return pattern_.c_str();
}

RegExpContext::RegExpContext()
    : regexMatch_(nullptr) {
}

RegExpContext::~RegExpContext() {
  if (regexMatch_ != nullptr)
    delete regexMatch_;
}

}  // namespace xzero
