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
#include <zbase/core/PartitionWriter.h>
#include <stx/util/PersistentHashSet.h>

using namespace stx;

namespace zbase {

class LSMPartitionWriter : public PartitionWriter {
public:
  static const size_t kDefaultMaxDatafileSize = 1024 * 1024 * 128;

  LSMPartitionWriter(PartitionSnapshotRef* head);

  Set<SHA1Hash> insertRecords(
      const Vector<RecordRef>& records) override;

  bool needsCompaction() override;

  void compact() override;

protected:
  PersistentHashSet idset_;
  size_t max_datafile_size_;
};

} // namespace tdsb

