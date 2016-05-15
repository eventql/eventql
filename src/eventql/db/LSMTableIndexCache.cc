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
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/logging.h>
#include <eventql/db/LSMTableIndexCache.h>

#include "eventql/eventql.h"

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
    slot = new Entry();
    slot->filename = filename;
    slot->idx = new LSMTableIndex();
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

  if (idx->load(FileUtil::joinPaths(base_path_, filename + ".idx"))) {
    size_ += idx->size() + filename.size() * 2 + kConstantOverhead;
  }

  if (size_ > max_size_) {
    lk.lock();
    while (size_ > max_size_) {
      flushTail();
    }
  }

  return idx;
}

void LSMTableIndexCache::flush(const String& filename) {
  ScopedLock<std::mutex> lk(mutex_);

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

  size_ -= (slot->idx->size() + slot->filename.size() * 2 + kConstantOverhead);
  delete slot;
}

// PRECONDITION: must hold mutex
void LSMTableIndexCache::flushTail() {
  auto entry = tail_;
  if (!entry) {
    return;
  }

  if (entry->prev) {
    entry->prev->next = nullptr;
  } else {
    head_ = nullptr;
  }

  tail_ = entry->prev;
  size_ -= (entry->idx->size() + entry->filename.size() * 2 + kConstantOverhead);
  map_.erase(entry->filename);
  delete entry;
}

size_t LSMTableIndexCache::size() const {
  return size_;
}

} // namespace eventql

