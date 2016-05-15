/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <stdlib.h>
#include <string>
#include "eventql/util/inspect.h"
#include "eventql/util/stringutil.h"
#include "eventql/util/json/jsonpointer.h"

namespace stx {
namespace json {

JSONPointer::JSONPointer() : path_("") {}

JSONPointer::JSONPointer(const char* path) : path_(path) {}

JSONPointer::JSONPointer(std::string path) : path_(path) {}

void JSONPointer::escape(std::string* str) {
  StringUtil::replaceAll(str, "~", "~0");
  StringUtil::replaceAll(str, "/", "~1");
}

void JSONPointer::push(std::string str) {
  JSONPointer::escape(&str);
  path_ += "/";
  path_ += str;
}

void JSONPointer::pop() {
  auto cur = path_.end() - 1;
  for (; cur >= path_.begin() && *cur != '/'; cur--);
  path_.erase(cur, path_.end());
}

std::string JSONPointer::head() const {
  auto cur = path_.end() - 1;
  for (; cur >= path_.begin() && *cur != '/'; cur--);
  return std::string(cur + 1, path_.end());
}


} // namespace json

template <>
std::string inspect(const json::JSONPointer& ptr) {
  return ptr.toString();
}

} // namespace stx

