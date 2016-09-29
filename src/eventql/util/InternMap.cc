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
#include <string.h>
#include "eventql/util/exception.h"
#include "eventql/util/InternMap.h"

void* InternMap::internString(const String& str) {
  std::unique_lock<std::mutex> lk(intern_map_mutex_);

  auto iter = intern_map_.find(str);
  if (iter != intern_map_.end()) {
    return iter->second;
  }

  char* istr_raw = (char*) malloc(sizeof(InternedStringHeader) + str.length());
  if (istr_raw == nullptr) {
    RAISE(kMallocError, "malloc() failed");
  }

  InternedStringHeader* istr = (InternedStringHeader*) istr_raw;
  istr->magic = kMagic;
  istr->size = str.length();
  memcpy(istr_raw + sizeof(InternedStringHeader), str.c_str(), str.length());
  intern_map_[str] = istr_raw;
  return istr_raw;
}

String InternMap::getString(const void* interned) {
  if (((InternedStringHeader*) interned)->magic != kMagic) {
    RAISEF(kRuntimeError, "invalid interned string: $0", interned);
  }

  auto data = ((char*) interned) + sizeof(InternedStringHeader);
  auto size = ((InternedStringHeader*) interned)->size;

  return std::string(data, size);
}
