/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_JSON_TOKENIZER_H
#define _STX_JSON_TOKENIZER_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <functional>
#include "stx/io/inputstream.h"
#include "stx/json/json.h"
#include "stx/json/jsonpointer.h"

namespace stx {
namespace json {

class JSONInputStream {
public:

  explicit JSONInputStream(std::unique_ptr<InputStream> input);
  JSONInputStream(JSONInputStream&& other);
  JSONInputStream(const JSONInputStream& other) = delete;
  JSONInputStream& operator=(const JSONInputStream& other) = delete;

  bool readNextToken(
      kTokenType* token_type,
      std::string* token_data);

protected:
  void readNumber(std::string* dst);
  void readString(std::string* dst);

  void expectString(const std::string& expect);
  void advanceCursor();

  std::unique_ptr<InputStream> input_;
  char cur_;
};

}
}
#endif
