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
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <stx/protobuf/MessageObject.h>
#include <zbase/core/RecordRef.h>

using namespace stx;

namespace zbase {

class RecordArena : public RefCounted {
public:

  RecordArena();

  bool insertRecord(const RecordRef& record);

  void fetchRecords(Function<void (const RecordRef& record)> fn);

  size_t size() const;

protected:
  HashMap<SHA1Hash, RecordRef> records_;
  mutable std::mutex mutex_;
};

} // namespace zbase

