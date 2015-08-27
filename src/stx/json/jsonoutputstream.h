/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_JSON_JSONOUTPUTSTREAM_H
#define _STX_JSON_JSONOUTPUTSTREAM_H
#include <set>
#include <stack>
#include <vector>
#include <stx/json/jsontypes.h>
#include <stx/exception.h>
#include <stx/io/outputstream.h>

namespace stx {
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
  std::string escapeString(const std::string& string) const;
  std::shared_ptr<OutputStream> output_;
};

} // namespace json
} // namespace stx

//#include "jsonoutputstream_impl.h"
#endif
