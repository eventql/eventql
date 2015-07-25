/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_JSON_H
#define _STX_JSON_H
#include <string>
#include <unordered_map>
#include <vector>
#include "stx/stdtypes.h"
#include "stx/buffer.h"
#include "stx/option.h"
#include "stx/json/jsontypes.h"
#include "stx/reflect/reflect.h"

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
