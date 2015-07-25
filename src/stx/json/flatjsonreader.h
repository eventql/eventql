/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_JSON_FLATJSONREADER_H
#define _STX_JSON_FLATJSONREADER_H
#include <stdlib.h>
#include <string>
#include "stx/json/json.h"
#include "stx/json/jsoninputstream.h"

namespace stx {
namespace json {

class FlatJSONReader {
public:
  explicit FlatJSONReader(JSONInputStream&& json);

  void read(
      std::function<bool (const JSONPointer&, const std::string&)> func);

protected:

  void readObject(
      std::function<bool (const JSONPointer&, const std::string&)> func,
      JSONPointer* path);

  void readArray(
      std::function<bool (const JSONPointer&, const std::string&)> func,
      JSONPointer* path);

  JSONInputStream json_;
};

} // namespace json
} // namespace stx
#endif
