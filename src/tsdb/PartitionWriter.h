/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <tsdb/PartitionSnapshot.h>
#include <tsdb/RecordRef.h>
#include <cstable/CSTableBuilder.h>

using namespace stx;

namespace tsdb {

class PartitionWriter : public RefCounted {
public:
  static const size_t kDefaultMaxDatafileSize = 1024 * 1024 * 128;

  PartitionWriter(PartitionSnapshotRef* head);

  virtual bool insertRecord(
      const SHA1Hash& record_id,
      const Buffer& record) = 0;

  virtual Set<SHA1Hash> insertRecords(
      const Vector<RecordRef>& records) = 0;

  virtual void updateCSTable(cstable::CSTableBuilder* cstable) = 0;

protected:
  PartitionSnapshotRef* head_;
  std::mutex mutex_;
};

} // namespace tdsb

