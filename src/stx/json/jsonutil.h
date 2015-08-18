/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_JSONUTIL_H
#define _STX_JSONUTIL_H
#include "stx/option.h"
#include "stx/json/jsontypes.h"

namespace stx {
namespace json {

JSONObject::const_iterator objectLookup(
    const JSONObject& obj,
    const std::string& key);

JSONObject::const_iterator objectLookup(
    JSONObject::const_iterator begin,
    JSONObject::const_iterator end,
    const std::string& key);

Option<String> objectGetString(
    const JSONObject& obj,
    const std::string& key);

Option<String> objectGetString(
    JSONObject::const_iterator begin,
    JSONObject::const_iterator end,
    const std::string& key);

Option<uint64_t> objectGetUInt64(
    const JSONObject& obj,
    const std::string& key);

Option<uint64_t> objectGetUInt64(
    JSONObject::const_iterator begin,
    JSONObject::const_iterator end,
    const std::string& key);

Option<bool> objectGetBool(
    const JSONObject& obj,
    const std::string& key);

Option<bool> objectGetBool(
    JSONObject::const_iterator begin,
    JSONObject::const_iterator end,
    const std::string& key);

size_t arrayLength(const JSONObject& obj);

size_t arrayLength(
    JSONObject::const_iterator begin,
    JSONObject::const_iterator end);

JSONObject::const_iterator arrayLookup(
    const JSONObject& obj,
    size_t index);

JSONObject::const_iterator arrayLookup(
    JSONObject::const_iterator begin,
    JSONObject::const_iterator end,
    size_t index);

Option<String> arrayGetString(
    const JSONObject& obj,
    size_t index);

Option<String> arrayGetString(
    JSONObject::const_iterator begin,
    JSONObject::const_iterator end,
    size_t index);

class JSONUtil {
public:

  static JSONObject::const_iterator objectLookup(
      JSONObject::const_iterator begin,
      JSONObject::const_iterator end,
      const std::string& key);

  static Option<String> objectGetString(
      JSONObject::const_iterator begin,
      JSONObject::const_iterator end,
      const std::string& key);

  static Option<uint64_t> objectGetUInt64(
      JSONObject::const_iterator begin,
      JSONObject::const_iterator end,
      const std::string& key);

  static Option<bool> objectGetBool(
      JSONObject::const_iterator begin,
      JSONObject::const_iterator end,
      const std::string& key);

  static size_t arrayLength(
      JSONObject::const_iterator begin,
      JSONObject::const_iterator end);

  static JSONObject::const_iterator arrayLookup(
      JSONObject::const_iterator begin,
      JSONObject::const_iterator end,
      size_t index);

  static Option<String> arrayGetString(
      JSONObject::const_iterator begin,
      JSONObject::const_iterator end,
      size_t index);

};

}
}

#endif
