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

  iputs("arena size: $0", records_.size());
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

} // namespace zbase

