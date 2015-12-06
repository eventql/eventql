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
    max_datafile_size_(kDefaultMaxDatafileSize) {}

Set<SHA1Hash> LSMPartitionWriter::insertRecords(const Vector<RecordRef>& records) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (frozen_) {
    RAISE(kIllegalStateError, "partition is frozen");
  }

  auto snap = head_->getSnapshot();

  stx::logTrace(
      "tsdb",
      "Insert $0 record into partition $1/$2/$3",
      records.size(),
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      snap->key.toString());

  Set<SHA1Hash> inserted_ids;
  for (const auto& r : records) {
    if (snap->head_arena->insertRecord(r)) {
      inserted_ids.emplace(r.record_id);
    }
  }

  iputs("insert...", 1);
  return Set<SHA1Hash>();
}

bool LSMPartitionWriter::needsCompaction() {
  auto snap = head_->getSnapshot();
  if (snap->compacting_arena.get() != nullptr ||
      snap->head_arena->size() > 0) {
    return true;
  }

  return false;
}

void LSMPartitionWriter::compact() {
  ScopedLock<std::mutex> compaction_lk(compaction_mutex_);

  // flip arenas
  auto snap = head_->getSnapshot();
  if (snap->compacting_arena.get() == nullptr) {
    ScopedLock<std::mutex> write_lk(mutex_);
    snap = snap->clone();
    snap->compacting_arena = snap->head_arena;
    snap->head_arena = mkRef(new RecordArena());
    head_->setSnapshot(snap);
  }

  iputs("compact... $0 records", snap->compacting_arena->size());

  // commit compaction
  {
    ScopedLock<std::mutex> write_lk(mutex_);
    snap = snap->clone();
    snap->compacting_arena = nullptr;
    head_->setSnapshot(snap);
  }
}

} // namespace tdsb
