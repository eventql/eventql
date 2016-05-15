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
#ifndef _STX_JSON_DOCUMENT_H
#define _STX_JSON_DOCUMENT_H
#include <functional>
#include <stdlib.h>
#include <string>
#include <vector>
#include "eventql/util/json/jsoninputstream.h"
#include "eventql/util/json/jsonpointer.h"

namespace util {
namespace json {

class JSONDocument {
public:

  explicit JSONDocument(std::string json);
  explicit JSONDocument(std::unique_ptr<InputStream> json);
  explicit JSONDocument(std::unique_ptr<JSONInputStream> json);
  explicit JSONDocument(JSONInputStream&& json);

  JSONDocument(const JSONDocument& other) = delete;
  JSONDocument& operator=(const JSONDocument& other) = delete;

  bool getMaybe(const JSONPointer& path, std::string* dst) const;
  std::string get(const JSONPointer& path) const;
  std::string get(const JSONPointer& path, const std::string& fallback) const;

  template <typename T>
  bool getMaybeAs(const JSONPointer& path, T* dst) const;

  template <typename T>
  T getAs(const JSONPointer& path) const;

  template <typename T>
  T getAs(const JSONPointer& path, const T& fallback) const;

  void forEach(
      const JSONPointer& path,
      std::function<bool (const JSONPointer& path)> callback) const;

protected:
  std::vector<std::pair<JSONPointer, std::string>> data_; // FIXPAUL!!!
};

} // namespace json
} // namespace util

#include "jsondocument_impl.h"
#endif
