/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/util/io/fileutil.h>
#include <eventql/core/Partition.h>
#include <eventql/core/PartitionWriter.h>
#include <eventql/util/logging.h>
#include <eventql/infra/sstable/SSTableWriter.h>

using namespace stx;

namespace zbase {

PartitionWriter::PartitionWriter(
    PartitionSnapshotRef* head) :
    head_(head),
    frozen_(false) {}

bool PartitionWriter::insertRecord(
    const SHA1Hash& record_id,
    uint64_t record_version,
    const Buffer& record) {
  Vector<RecordRef> recs;
  recs.emplace_back(record_id, record_version, record);
  auto ids = insertRecords(recs);
  return !ids.empty();
}

void PartitionWriter::updateCSTable(
    const String& tmpfile,
    uint64_t version) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (frozen_) {
    RAISE(kIllegalStateError, "partition is frozen");
  }

  auto snap = head_->getSnapshot()->clone();

  if (snap->state.cstable_version() >= version) {
    RAISE(
        kRuntimeError,
        "refusing cstable update because the update version is less than or " \
        "equal to the head version");
  }

  logDebug(
      "tsdb",
      "Updating cstable for partition $0/$1/$2",
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      snap->key.toString());

  auto filepath = FileUtil::joinPaths(snap->base_path, "_cstable");
  FileUtil::mv(tmpfile, filepath);

  snap->state.set_cstable_version(version);
  snap->writeToDisk();
  head_->setSnapshot(snap);
}

void PartitionWriter::lock() {
  mutex_.lock();
}

void PartitionWriter::unlock() {
  mutex_.unlock();
}

void PartitionWriter::freeze() {
  frozen_ = true;
}

} // namespace tdsb
