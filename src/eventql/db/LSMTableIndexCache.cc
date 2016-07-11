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
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/logging.h>
#include <eventql/db/LSMTableIndexCache.h>
#include "eventql/eventql.h"
#include <assert.h>

namespace eventql {

LSMTableIndexCache::LSMTableIndexCache(
    const String& base_path,
    size_t max_size /* = kDefaultMaxSize */) :
    base_path_(base_path),
    head_(nullptr),
    tail_(nullptr),
    size_(0),
    max_size_(max_size) {}

LSMTableIndexCache::~LSMTableIndexCache() {
  for (const auto& slot : map_) {
    delete slot.second;
  }
}

RefPtr<LSMTableIndex> LSMTableIndexCache::lookup(const String& filename) {
  auto file_path = FileUtil::joinPaths(base_path_, filename + ".idx");

  ScopedLock<std::mutex> lk(mutex_);
  auto& slot = map_[filename];
  if (slot) {
    if (slot->next) {
      slot->next->prev = slot->prev;
    } else {
      tail_ = slot->prev;
    }

    if (slot->prev) {
      slot->prev->next = slot->next;
    } else {
      head_ = slot->next;
    }
  } else {
    while (size_ > max_size_) {
      flushTail();
    }

    slot = new Entry();
    slot->filename = filename;
    slot->idx = new LSMTableIndex();
    slot->size =
        FileUtil::size(file_path) + filename.size() * 2 + kConstantOverhead;
    size_ += slot->size;

    logInfo(
        "evqld",
        "LSMTableIndexCache::load: $0 (size=$1/$2)",
        filename,
        size_,
        slot->size);
  }

  slot->next = head_;
  slot->prev = nullptr;
  if (head_) {
    head_->prev = slot;
  } else {
    tail_ = slot;
  }

  head_ = slot;
  auto idx = slot->idx;
  lk.unlock();

  idx->load(file_path);

  return idx;
}

void LSMTableIndexCache::flush(const String& filename) {
  ScopedLock<std::mutex> lk(mutex_);
  logInfo("evqld", "LSMTableIndexCache::flush: $0 (size=$1)", filename, size_);

  auto iter = map_.find(filename);
  if (iter == map_.end()) {
    return;
  }

  auto slot = iter->second;
  map_.erase(iter);

  if (slot->next) {
    slot->next->prev = slot->prev;
  } else {
    tail_ = slot->prev;
  }

  if (slot->prev) {
    slot->prev->next = slot->next;
  } else {
    head_ = slot->next;
  }

  size_ -= slot->size;
  assert(head_ || size_ == 0);
  delete slot;
}

// PRECONDITION: must hold mutex
void LSMTableIndexCache::flushTail() {
  auto entry = tail_;
  assert(entry != nullptr);

  if (entry->prev) {
    entry->prev->next = nullptr;
  } else {
    head_ = nullptr;
  }

  tail_ = entry->prev;
  size_ -= entry->size;
  map_.erase(entry->filename);
  assert(head_ || size_ == 0);
  delete entry;
}

size_t LSMTableIndexCache::size() const {
  ScopedLock<std::mutex> lk(mutex_);
  return size_;
}

} // namespace eventql

