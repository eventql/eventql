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
#include "eventql/util/json/jsonutil.h"

namespace json {

template <typename T>
T JSONRPCRequest::getArg(size_t index, const std::string& name) const {
  auto begin = paramsBegin();
  auto end = paramsEnd();

  switch (begin->type) {
    case json::JSON_OBJECT_BEGIN: {
      auto iter = JSONUtil::objectLookup(begin, end, name);
      if (iter == end) {
        RAISEF(kIndexError, "missing argument: $0", name);
      }

      return fromJSON<T>(iter, end);
    }

    case json::JSON_ARRAY_BEGIN: {
      JSONObject::const_iterator iter;
      try {
        iter = JSONUtil::arrayLookup(begin, end, index);
      } catch (const std::exception& e) {
        RAISEF(kIndexError, "missing argument: $0", name);
      }

      return fromJSON<T>(iter, end);
    }

    default:
      RAISE(kParseError, "params type must be array or object");

  }
}

} // namespace json
