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
#include <eventql/util/inspect.h>

template <>
std::string inspect<bool>(const bool& value) {
  return value == true ? "true" : "false";
}

template <>
std::string inspect<int>(const int& value) {
  return std::to_string(value);
}

template <>
std::string inspect<unsigned int>(const unsigned int& value) {
  return std::to_string(value);
}

template <>
std::string inspect<unsigned long>(const unsigned long& value) {
  return std::to_string(value);
}

template <>
std::string inspect<unsigned long long>(const unsigned long long& value) {
  return std::to_string(value);
}

template <>
std::string inspect<unsigned char>(const unsigned char& value) {
  return std::to_string(value);
}

template <>
std::string inspect<long long>(
    const long long& value) {
  return std::to_string(value);
}

template <>
std::string inspect<long>(
    const long& value) {
  return std::to_string(value);
}

template <>
std::string inspect<unsigned short>(
    const unsigned short& value) {
  return std::to_string(value);
}

template <>
std::string inspect<float>(const float& value) {
  return std::to_string(value);
}

template <>
std::string inspect<double>(const double& value) {
  return std::to_string(value);
}

template <>
std::string inspect<std::string>(const std::string& value) {
  return value;
}

template <>
std::string inspect<std::wstring>(const std::wstring& value) {
  std::string out;
  out.assign(value.begin(), value.end());
  return out;
}

template <>
std::string inspect<char const*>(char const* const& value) {
  return std::string(value);
}

template <>
std::string inspect<void*>(void* const& value) {
  return "<ptr>";
}

template <>
std::string inspect<const void*>(void const* const& value) {
  return "<ptr>";
}

template <>
std::string inspect<std::exception>(const std::exception& e) {
  return e.what();
}

