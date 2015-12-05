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
#include <zbase/core/LSMPartitionWriter.h>
#include <stx/logging.h>
#include <sstable/SSTableWriter.h>

using namespace stx;

namespace zbase {

LSMPartitionWriter::LSMPartitionWriter(
    PartitionSnapshotRef* head) :
    PartitionWriter(head),
    idset_(FileUtil::joinPaths(head_->getSnapshot()->base_path, "_idset")),
    max_datafile_size_(kDefaultMaxDatafileSize) {}

Set<SHA1Hash> LSMPartitionWriter::insertRecords(const Vector<RecordRef>& records) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (frozen_) {
    RAISE(kIllegalStateError, "partition is frozen");
  }

  auto snap = head_->getSnapshot()->clone();

  stx::logTrace(
      "tsdb",
      "Insert $0 record into partition $1/$2/$3",
      records.size(),
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      snap->key.toString());

  Set<SHA1Hash> record_ids;
  for (const auto& r : records) {
    record_ids.emplace(r.record_id);
  }

  idset_.addRecordIDs(&record_ids);

  if (record_ids.empty()) {
    return record_ids;
  }

  bool snap_dirty = false;
  ScopedPtr<sstable::SSTableWriter> writer;
  if (snap->state.sstable_files().size() == 0) {
    snap->state.add_sstable_files("_log");
    snap_dirty = true;

    writer = sstable::SSTableWriter::create(
        FileUtil::joinPaths(snap->base_path, "_log"), nullptr, 0);
  } else {
    writer = sstable::SSTableWriter::reopen(
        FileUtil::joinPaths(
            snap->base_path,
            *(snap->state.sstable_files().end() - 1)));
  }

  for (const auto& r : records) {
    if (record_ids.count(r.record_id) == 0) {
      continue;
    }

    writer->appendRow(
        r.record_id.data(),
        r.record_id.size(),
        r.record.data(),
        r.record.size());
  }

  writer->commit();

  snap->nrecs += record_ids.size();
  if (snap_dirty) {
    snap->writeToDisk();
  }

  head_->setSnapshot(snap);

  return record_ids;
}

bool LSMPartitionWriter::needsCompaction() {
  return false;
}

void LSMPartitionWriter::compact() {

}

} // namespace tdsb
