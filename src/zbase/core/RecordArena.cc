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
#include <zbase/core/RecordArena.h>

using namespace stx;

namespace zbase {

RecordArena::RecordArena() {}

bool RecordArena::insertRecord(const RecordRef& record) {
  ScopedLock<std::mutex> lk(mutex_);

  auto old = records_.find(record.record_id);
  if (old == records_.end()) {
    records_.emplace(record.record_id, record);
    return true;
  } else if(old->second.record_version < record.record_version) {
    old->second = record;
    return true;
  } else {
    return false;
  }
}


void RecordArena::fetchRecords(Function<void (const RecordRef& record)> fn) {
  ScopedLock<std::mutex> lk(mutex_); // FIXME

  for (const auto& r : records_) {
    fn(r.second);
  }
}

size_t RecordArena::size() const {
  ScopedLock<std::mutex> lk(mutex_);
  return records_.size();
}

} // namespace zbase

