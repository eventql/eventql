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
#include <tsdb/Partition.h>
#include <tsdb/PartitionWriter.h>
#include <stx/logging.h>
#include <sstable/SSTableWriter.h>

using namespace stx;

namespace tsdb {

PartitionWriter::PartitionWriter(
    PartitionSnapshotRef* head) :
    head_(head) {}

void PartitionWriter::updateCSTable(
    const String& tmpfile,
    uint64_t version) {
  std::unique_lock<std::mutex> lk(mutex_);
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

} // namespace tdsb
