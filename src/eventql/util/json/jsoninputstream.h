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
#ifndef _STX_JSON_TOKENIZER_H
#define _STX_JSON_TOKENIZER_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <functional>
#include "eventql/util/io/inputstream.h"
#include "eventql/util/json/json.h"
#include "eventql/util/json/jsonpointer.h"

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
#endif
