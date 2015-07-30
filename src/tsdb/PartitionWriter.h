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
#include <tsdb/RecordRef.h>

using namespace stx;

namespace tsdb {
class Partition;

struct PartitionWriter : public RefCounted {

  PartitionWriter(Partition* partition);

  void insertRecord(
      const SHA1Hash& record_id,
      const Buffer& record);

  void insertRecords(
      const Vector<RecordRef>& records);

protected:
  Partition* partition_;
  RecordIDSet recids_;
  std::mutex mutex_;
};

} // namespace tdsb

