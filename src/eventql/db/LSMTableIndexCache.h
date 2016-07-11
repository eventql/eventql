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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/db/LSMTableIndex.h>

#include "eventql/eventql.h"

namespace eventql {

class LSMTableIndexCache : public RefCounted {
public:
  static const size_t kDefaultMaxSize = 1024 * 1024 * 1024; // 1024 MB;

  LSMTableIndexCache(
      const String& base_path,
      size_t max_size = kDefaultMaxSize);

  ~LSMTableIndexCache();

  RefPtr<LSMTableIndex> lookup(const String& filename);

  void flush(const String& filename);

  size_t size() const;

protected:

  static const size_t kConstantOverhead = 32;

  struct Entry {
    RefPtr<LSMTableIndex> idx;
    String filename;
    Entry* prev;
    Entry* next;
    size_t size;
  };

  void flushTail();

  const String base_path_;
  mutable std::mutex mutex_;
  HashMap<String, Entry*> map_;
  Entry* head_;
  Entry* tail_;
  size_t size_;
  size_t max_size_;
};

} // namespace eventql

