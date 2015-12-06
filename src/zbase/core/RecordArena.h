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
#include <zbase/core/PartitionWriter.h>
#include <stx/util/PersistentHashSet.h>
#include <stx/protobuf/MessageObject.h>

using namespace stx;

namespace zbase {

class RecordArena : public RefCounted {
public:

  RecordArena();

  bool insertRecord(const RecordRef& record);

protected:
  HashMap<SHA1Hash, RecordRef> records_;
  std::mutex mutex_;
};

} // namespace zbase

