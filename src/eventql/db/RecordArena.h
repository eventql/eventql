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
#include <eventql/util/protobuf/MessageObject.h>
#include <eventql/db/RecordRef.h>

#include "eventql/eventql.h"

namespace eventql {

class RecordArena : public RefCounted {
public:

  RecordArena();

  bool insertRecord(const RecordRef& record);

  void fetchRecords(Function<void (const RecordRef& record)> fn);

  uint64_t fetchRecordVersion(const SHA1Hash& record_id);

  size_t size() const;

protected:
  HashMap<SHA1Hash, RecordRef> records_;
  mutable std::mutex mutex_;
};

} // namespace eventql

