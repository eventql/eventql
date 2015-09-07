/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <string>
#include <stx/bufferutil.h>
#include <stx/stringutil.h>
#include <stx/UTF8.h>

namespace stx {

void StringUtil::toStringVImpl(std::vector<std::string>* target) {}

template <>
std::string StringUtil::toString(std::string value) {
  return value;
}

template <>
std::string StringUtil::toString(const char* value) {
  return value;
}

template <>
std::string StringUtil::toString(char* value) {
  return value;
}

template <>
std::string StringUtil::toString(int value) {
  return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned value) {
  return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned short value) {
  return std::to_string(value);
}


template <>
std::string StringUtil::toString(long value) {
  return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned long value) {
  return std::to_string(value);
}

template <>
std::string StringUtil::toString(long long value) {
  return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned long long value) {
  return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned char value) {
  return std::to_string(value);
}

template <>
std::string StringUtil::toString(void* value) {
  return "<ptr>";
}

template <>
std::string StringUtil::toString(const void* value) {
  return "<ptr>";
}

template <>
std::string StringUtil::toString(double value) {
  char buf[128]; // FIXPAUL
  *buf = 0;

  auto len = snprintf(buf, sizeof(buf), "%f", value);
  if (len < 0) {
    RAISE(kRuntimeError, "snprintf() failed");
  }

  while (len > 2 && buf[len - 1] == '0' && buf[len - 2] != '.') {
    buf[len--] = 0;
  }

  return String(buf, len);
}

template <>
std::string StringUtil::toString(bool value) {
  return value ? "true" : "false";
}

void StringUtil::stripTrailingSlashes(std::string* str) {
  while (str->back() == '/') {
    str->pop_back();
  }
}

void StringUtil::replaceAll(
    std::string* str,
    const std::string& pattern,
    const std::string& replacement) {
  if (str->size() == 0) {
    return;
  }

  auto cur = 0;
  while((cur = str->find(pattern, cur)) != std::string::npos) {
    str->replace(cur, pattern.size(), replacement);
    cur += replacement.size();
  }
}

std::vector<std::string> StringUtil::split(
      const std::string& str,
      const std::string& pattern) {
  std::vector<std::string> parts;

  size_t begin = 0;
  for (;;) {
    auto end = str.find(pattern, begin);

    if (end == std::string::npos) {
      parts.emplace_back(str.substr(begin, end));
      break;
    } else {
      parts.emplace_back(str.substr(begin, end - begin));
      begin = end + pattern.length();
    }
  }

  return parts;
}

String StringUtil::join(const Vector<String>& list, const String& join) {
  String out;

  for (int i = 0; i < list.size(); ++i) {
    if (i > 0) {
      out += join;
    }

    out += list[i];
  }

  return out;
}

String StringUtil::join(const Set<String>& list, const String& join) {
  String out;

  size_t i = 0;
  for (const auto& item : list) {
    if (++i > 1) {
      out += join;
    }

    out += item;
  }

  return out;
}

bool StringUtil::beginsWith(const std::string& str, const std::string& prefix) {
  if (str.length() < prefix.length()) {
    return false;
  }

  return str.compare(
      0,
      prefix.length(),
      prefix) == 0;
}


bool StringUtil::endsWith(const std::string& str, const std::string& suffix) {
  if (str.length() < suffix.length()) {
    return false;
  }

  return str.compare(
      str.length() - suffix.length(),
      suffix.length(),
      suffix) == 0;
}

int StringUtil::compare(
    const char* s1,
    size_t s1_len,
    const char* s2,
    size_t s2_len) {
  for (; s1_len > 0 && s2_len > 0; s1++, s2++, --s1_len, --s2_len) {
    if (*s1 != *s2) {
      return (*(uint8_t *) s1 < *(uint8_t *) s2) ? -1 : 1;
    }
  }

  if (s1_len > 0) {
    return 1;
  }

  if (s2_len > 0) {
    return -1;
  }

  return 0;
}


bool StringUtil::isHexString(const std::string& str) {
  for (const auto& c : str) {
    if ((c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'f') ||
        (c >= 'A' && c <= 'F')) {
      continue;
    }

    return false;
  }

  return true;
}

bool StringUtil::isAlphanumeric(const std::string& str) {
  for (const auto& c : str) {
    if (!isAlphanumeric(c)) {
      return false;
    }
  }

  return true;
}

bool StringUtil::isAlphanumeric(char chr) {
  bool is_alphanum =
      (chr >= '0' && chr <= '9') ||
      (chr >= 'a' && chr <= 'z') ||
      (chr >= 'A' && chr <= 'Z');

  return is_alphanum;
}

bool StringUtil::isShellSafe(const std::string& str) {
  for (const auto& c : str) {
    if (!isShellSafe(c)) {
      return false;
    }
  }

  return true;
}

bool StringUtil::isShellSafe(char chr) {
  bool is_safe =
      (chr >= '0' && chr <= '9') ||
      (chr >= 'a' && chr <= 'z') ||
      (chr >= 'A' && chr <= 'Z') ||
      (chr == '_') ||
      (chr == '-') ||
      (chr == '.');

  return is_safe;
}

bool StringUtil::isDigitString(const std::string& str) {
  return isDigitString(str.data(), str.data() + str.size());
}

bool StringUtil::isDigitString(const char* begin, const char* end) {
  for (auto cur = begin; cur < end; ++cur) {
    if (!isdigit(*cur)) {
      return false;
    }
  }

  return true;
}

bool StringUtil::isNumber(const std::string& str) {
  return isNumber(str.data(), str.data() + str.size());
}

bool StringUtil::isNumber(const char* begin, const char* end) {
  auto cur = begin;

  if (cur < end && *cur == '-') {
    ++cur;
  }

  for (; cur < end; ++cur) {
    if (!isdigit(*cur)) {
      return false;
    }
  }

  if (cur < end && (*cur == '.' || *cur == ',')) {
    ++cur;
  }

  for (; cur < end; ++cur) {
    if (!isdigit(*cur)) {
      return false;
    }
  }

  return true;
}

void StringUtil::toLower(std::string* str) {
  auto& str_ref = *str;

  for (int i = 0; i < str_ref.length(); ++i) {
    str_ref[i] = std::tolower(str_ref[i]);
  }
}

void StringUtil::toUpper(std::string* str) {
  auto& str_ref = *str;

  for (int i = 0; i < str_ref.length(); ++i) {
    str_ref[i] = std::toupper(str_ref[i]);
  }
}

size_t StringUtil::find(const std::string& str, char chr) {
  for (int i = 0; i < str.length(); ++i) {
    if (str[i] == chr) {
      return i;
    }
  }

  return -1;
}

size_t StringUtil::findLast(const std::string& str, char chr) {
  for (int i = str.length() - 1; i >= 0; --i) {
    if (str[i] == chr) {
      return i;
    }
  }

  return -1;
}

bool StringUtil::includes(const std::string& str, const std::string& subject) {
  return str.find(subject) != std::string::npos;
}

std::string StringUtil::hexPrint(
    const void* data,
    size_t size,
    bool sep /* = true */,
    bool reverse /* = fase */) {
  Buffer buf(data, size);
  return BufferUtil::hexPrint(&buf, sep, reverse);
}

std::string StringUtil::formatv(
    const char* fmt,
    std::vector<std::string> values) {
  std::string str = fmt;

  for (int i = 0; i < values.size(); ++i) {
    StringUtil::replaceAll(
        &str,
        "$" + std::to_string(i),
        StringUtil::toString(values[i]));
  }

  return str;
}

std::wstring StringUtil::convertUTF8To16(const std::string& str) {
  WString out;

  const char* cur = str.data();
  const char* end = cur + str.length();
  char32_t chr;
  while ((chr = UTF8::nextCodepoint(&cur, end)) > 0) {
    out += (wchar_t) chr;
  }

  return out;
}

std::string StringUtil::convertUTF16To8(const std::wstring& str) {
  String out;

  for (const auto& c : str) {
    UTF8::encodeCodepoint(c, &out);
  }

  return out;
}

String StringUtil::stripShell(const std::string& str) {
  String out;

  for (const auto& c : str) {
    if (isAlphanumeric(c) || c == '_' || c == '-' || c == '.') {
      out += c;
    }
  }

  return out;
}

} // namespace stx
