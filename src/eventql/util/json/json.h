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
#ifndef _STX_JSON_H
#define _STX_JSON_H
#include <string>
#include <unordered_map>
#include <vector>
#include "eventql/util/stdtypes.h"
#include "eventql/util/buffer.h"
#include "eventql/util/option.h"
#include "eventql/util/json/jsontypes.h"
#include "eventql/util/reflect/reflect.h"

namespace stx {
namespace json {
class JSONOutputStream;
class JSONInputStream;

template <typename T, typename O>
void toJSONImpl(const Vector<T>& value, O* target);

template <typename T, typename O>
void toJSONImpl(const Set<T>& value, O* target);

template <typename T, typename O>
void toJSONImpl(const T& value, O* target);

template <typename T>
struct JSONInputProxy {
public:
  JSONInputProxy(
      JSONObject::const_iterator begin,
      JSONObject::const_iterator end);

  template <typename PropertyType>
  PropertyType getProperty(uint32_t id, const std::string& name);

  template <typename PropertyType>
  Option<PropertyType> getOptionalProperty(
      uint32_t id,
      const std::string& name);

  JSONObject::const_iterator obj_begin;
  JSONObject::const_iterator obj_end;
  T value;
};

template <typename OutputType>
struct JSONOutputProxy {
public:
  template <typename T>
  JSONOutputProxy(const T& instance, OutputType* target);

  template <typename T>
  void putProperty(uint32_t id, const std::string& name, const T& value);

  JSONObject object;
  OutputType* target_;
};

template <typename T>
JSONObject toJSON(const T& value);

template <typename T>
std::string toJSONString(const T& value);

template <typename T>
T fromJSON(const std::string& json_str);

template <typename T>
T fromJSON(const stx::Buffer& json_buf);

template <typename T>
T fromJSON(const JSONObject& jsonobj);

JSONObject parseJSON(const std::string& json_str);
JSONObject parseJSON(const stx::Buffer& json_buf);
JSONObject parseJSON(JSONInputStream* json);

}
}

#endif
#include "json_impl.h"
