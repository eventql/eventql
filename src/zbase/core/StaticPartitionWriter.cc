/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/io/fileutil.h>
#include <zbase/core/Partition.h>
#include <zbase/core/StaticPartitionWriter.h>
#include <stx/logging.h>
#include <sstable/SSTableWriter.h>

using namespace stx;

namespace zbase {

StaticPartitionWriter::StaticPartitionWriter(
    PartitionSnapshotRef* head) :
    PartitionWriter(head) {}

Set<SHA1Hash> StaticPartitionWriter::insertRecords(
    const Vector<RecordRef>& records) {
  RAISE(
      kRuntimeError,
      "can't insert individual records because partition is STATIC");
}

} // namespace tdsb
