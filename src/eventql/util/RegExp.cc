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
#include <eventql/util/RegExp.h>
#include <cstring>

#ifdef HAVE_PCRE
#include <pcre.h>
#endif

RegExp::RegExp() {
#ifdef HAVE_PCRE
  pcre_handle_ = nullptr;
#endif
}

RegExp::RegExp(const RegExp& other) :
  pattern_(other.pattern_) {
#ifdef HAVE_PCRE
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
#else
  re_ = other.re_;
#endif
}

RegExp::RegExp(const std::string& pattern) : pattern_(pattern) {
#ifdef HAVE_PCRE
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
#else
  re_ = std::regex(pattern);
#endif
}

RegExp::~RegExp() {
#ifdef HAVE_PCRE
  if (pcre_handle_) {
    pcre_free(pcre_handle_);
  }
#endif
}

RegExp::RegExp(
    RegExp&& other) :
    pattern_(std::move(other.pattern_)) {
#ifdef HAVE_PCRE
  pcre_handle_ = other.pcre_handle_;
  other.pcre_handle_ = nullptr;
#else
  re_ = std::move(other.re_);
#endif
}

RegExp& RegExp::operator=(RegExp&& other) {
  pattern_ = std::move(other.pattern_);

#ifdef HAVE_PCRE
  if (pcre_handle_) {
    pcre_free(pcre_handle_);
  }

  pcre_handle_ = other.pcre_handle_;
  other.pcre_handle_ = nullptr;
#else
  re_ = std::move(other.re_);
#endif

  return *this;
}

#ifdef HAVE_PCRE

bool RegExp::match(const char* buffer, size_t size, Result* result) const {
  if (!pcre_handle_) return false;

  const size_t OV_COUNT = 3 * 36;
  int ov[OV_COUNT];
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

#endif

const char* RegExp::c_str() const {
  return pattern_.c_str();
}
