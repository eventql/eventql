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
#ifndef _STX_JSON_JSONOUTPUTSTREAM_H
#define _STX_JSON_JSONOUTPUTSTREAM_H
#include <set>
#include <stack>
#include <vector>
#include <eventql/util/json/jsontypes.h>
#include <eventql/util/exception.h>
#include <eventql/util/io/outputstream.h>

namespace util {
namespace json {

class JSONOutputStream {
public:

  JSONOutputStream(std::unique_ptr<OutputStream> output_stream);

  void write(const JSONObject& obj);

  void emplace_back(kTokenType token);
  void emplace_back(kTokenType token, const std::string& data);
  void emplace_back(const JSONToken& token);

  void beginObject();
  void endObject();
  void addObjectEntry(const std::string& key);
  void beginArray();
  void endArray();
  void addComma();
  void addColon();
  void addString(const std::string& string);
  void addFloat(double value);
  void addInteger(int64_t value);
  void addNull();
  void addBool(bool val);
  void addTrue();
  void addFalse();


protected:
  std::stack<std::pair<kTokenType, int>> stack_;
  std::shared_ptr<OutputStream> output_;
};

std::string escapeString(const std::string& string);

} // namespace json
} // namespace util

//#include "jsonoutputstream_impl.h"
#endif
