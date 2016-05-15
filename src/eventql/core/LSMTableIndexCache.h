/**
 * Copyright (c) 2015 - zScale Technology GmbH <legal@zscale.io>
 *   All Rights Reserved.
 *
 * Authors:
 *   Paul Asmuth <paul@zscale.io>
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/core/LSMTableIndex.h>

using namespace stx;

namespace eventql {

class LSMTableIndexCache : public RefCounted {
public:
  static const size_t kDefaultMaxSize = 1024 * 1024 * 256; // 256 MB;

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
  };

  void flushTail();

  const String base_path_;
  std::mutex mutex_;
  HashMap<String, Entry*> map_;
  Entry* head_;
  Entry* tail_;
  std::atomic<size_t> size_;
  size_t max_size_;
};

} // namespace eventql

