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
#include <tsdb/PartitionWriter.h>
#include <stx/util/PersistentHashSet.h>

using namespace stx;

namespace tsdb {

class StaticPartitionWriter : public PartitionWriter {
public:

  StaticPartitionWriter(PartitionSnapshotRef* head);

  bool insertRecord(
      const SHA1Hash& record_id,
      const Buffer& record) override;

  Set<SHA1Hash> insertRecords(
      const Vector<RecordRef>& records) override;

  void updateCSTable(cstable::CSTableBuilder* cstable) override;

};

} // namespace tdsb

