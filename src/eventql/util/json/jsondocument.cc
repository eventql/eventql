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
#include <set>
#include "eventql/util/UnixTime.h"
#include "eventql/util/exception.h"
#include "eventql/util/stringutil.h"
#include "eventql/util/inspect.h"
#include "eventql/util/json/flatjsonreader.h"
#include "eventql/util/json/jsondocument.h"
#include "eventql/util/json/jsonpointer.h"

namespace json {

JSONDocument::JSONDocument(
    std::string json) :
    JSONDocument(JSONInputStream(StringInputStream::fromString(json))) {}

JSONDocument::JSONDocument(
    std::unique_ptr<InputStream> json) :
    JSONDocument(JSONInputStream(std::move(json))) {}

JSONDocument::JSONDocument(
    std::unique_ptr<JSONInputStream> json) :
    JSONDocument(std::move(*json)) {}

JSONDocument::JSONDocument(JSONInputStream&& json) {
  FlatJSONReader reader(std::move(json));

  reader.read([this] (
      const JSONPointer& path,
      const std::string& value) -> bool {
    data_.emplace_back(std::make_pair(path, value));
    return true;
  });
}

bool JSONDocument::getMaybe(const JSONPointer& path, std::string* dst) const {
  // FIXPAUL!!!
  for (const auto& pair : data_) {
    if (pair.first == path) {
      *dst = pair.second;
      return true;
    }
  }

  return false;
}

template <>
bool JSONDocument::getMaybeAs(const JSONPointer& path, double* dst) const {
  std::string value;

  if (!getMaybe(path, &value)) {
    return false;
  }

  try {
    *dst = std::stod(value);
  } catch (std::exception& e) {
    RAISEF(kIllegalArgumentError, "not a valid float: $0", value);
    return false;
  }

  return true;
}

template <>
bool JSONDocument::getMaybeAs(
    const JSONPointer& path,
    unsigned long* dst) const {
  std::string value;

  if (!getMaybe(path, &value)) {
    return false;
  }

  try {
    *dst = std::stoul(value);
  } catch (std::exception& e) {
    RAISEF(kIllegalArgumentError, "not a valid float: $0", value);
    return false;
  }

  return true;
}

template <>
bool JSONDocument::getMaybeAs(
    const JSONPointer& path,
    unsigned long long* dst) const {
  std::string value;

  if (!getMaybe(path, &value)) {
    return false;
  }

  try {
    *dst = std::stoull(value);
  } catch (std::exception& e) {
    RAISEF(kIllegalArgumentError, "not a valid float: $0", value);
    return false;
  }

  return true;
}

template <>
bool JSONDocument::getMaybeAs(const JSONPointer& path, UnixTime* dst) const {
  uint64_t val;

  if (!getMaybeAs<uint64_t>(path, &val)) {
    return false;
  }

  *dst = UnixTime(val);
  return true;
}

std::string JSONDocument::get(const JSONPointer& path) const {
  std::string value;

  if (!getMaybe(path, &value)) {
    RAISEF(kIndexError, "no such element: $0", path.toString());
  }

  return value;
}

std::string JSONDocument::get(
    const JSONPointer& path,
    const std::string& fallback) const {
  std::string value;

  if (getMaybe(path, &value)) {
    return value;
  } else {
    return fallback;
  }
}

// FIXPAUL!!!
void JSONDocument::forEach(
    const JSONPointer& path,
    std::function<bool (const JSONPointer& path)> callback) const {
  // FIXPAUL!!!
  std::set<std::string> children;

  auto prefix = path.toString();
  StringUtil::stripTrailingSlashes(&prefix);
  prefix += "/";

  // FIXPAUL!!!
  for (const auto& pair : data_) {
    auto elem_path = pair.first.toString();
    if (StringUtil::beginsWith(elem_path, prefix)) {
      for(auto cur = elem_path.begin() + prefix.length();
          cur < elem_path.end();
          ++cur) {
        if (*cur == '/') {
          elem_path.erase(cur, elem_path.end());
          break;
        }
      }

      children.insert(elem_path);
    }
  }

  for (const auto& child: children) {
    callback(JSONPointer(child));
  }
}

} // namespace json

