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
#ifndef _STX_JSONUTIL_H
#define _STX_JSONUTIL_H
#include "eventql/util/option.h"
#include "eventql/util/json/jsontypes.h"

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

Option<double> objectGetFloat(
    const JSONObject& obj,
    const std::string& key);

Option<double> objectGetFloat(
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

  static Option<double> objectGetFloat(
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

#endif
