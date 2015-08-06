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
#include <tsdb/LogPartitionWriter.h>
#include <stx/logging.h>
#include <sstable/SSTableWriter.h>

using namespace stx;

namespace tsdb {

LogPartitionWriter::LogPartitionWriter(
    PartitionSnapshotRef* head) :
    PartitionWriter(head),
    idset_(FileUtil::joinPaths(head_->getSnapshot()->base_path, "_idset")),
    max_datafile_size_(kDefaultMaxDatafileSize) {}

bool LogPartitionWriter::insertRecord(
    const SHA1Hash& record_id,
    const Buffer& record) {
  Vector<RecordRef> recs;
  recs.emplace_back(record_id, record);
  auto ids = insertRecords(recs);
  return !ids.empty();
}

Set<SHA1Hash> LogPartitionWriter::insertRecords(const Vector<RecordRef>& records) {
  std::unique_lock<std::mutex> lk(mutex_);
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

  String filename;
  const auto& old_files = snap->state.sstable_files();
  if (old_files.size() > 0) {
    auto last_file = *(old_files.end() + - 1);
    auto last_file_size = FileUtil::size(
        FileUtil::joinPaths(snap->base_path, last_file));

    if (last_file_size < max_datafile_size_) {
      filename = last_file;
    }
  }


  ScopedPtr<sstable::SSTableWriter> writer;
  bool snap_dirty = false;
  if (filename.empty()) {
    filename = StringUtil::format("$0.sst", Random::singleton()->hex64());
    snap->state.add_sstable_files(filename);
    writer = sstable::SSTableWriter::create(
        FileUtil::joinPaths(snap->base_path, filename), nullptr, 0);
    snap_dirty = true;
  } else {
    writer = sstable::SSTableWriter::reopen(
        FileUtil::joinPaths(snap->base_path, filename));
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

void LogPartitionWriter::updateCSTable(cstable::CSTableBuilder* cstable) {
  RAISE(kNotImplementedError);
}

} // namespace tdsb
