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

using namespace stx;

namespace tsdb {

PartitionWriter::PartitionWriter(
    RefPtr<PartitionSnapshot>* head) :
    head_(head),
    idset_(
        FileUtil::joinPaths(
            head_->get()->base_path,
            head_->get()->key.toString() + ".idx")) {}

bool PartitionWriter::insertRecord(
    const SHA1Hash& record_id,
    const Buffer& record) {
  Vector<RecordRef> recs;
  recs.emplace_back(record_id, record);
  auto ids = insertRecords(recs);
  return !ids.empty();
}

Set<SHA1Hash> PartitionWriter::insertRecords(const Vector<RecordRef>& records) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto snap = head_->get()->clone();

  stx::logTrace(
      "tsdb",
      "Insert $0 record into partition $1/$2/$3",
      records.size(),
      snap->state.tsdb_namespace(),
      snap->table->name(),
      snap->key.toString());

  Set<SHA1Hash> record_ids;
  idset_.addRecordIDs(&record_ids);

  if (record_ids.empty()) {
    return record_ids;
  }

  String filename;
  const auto& old_files = snap->state.sstable_files();
  if (old_files.size() == 0) {
    filename = StringUtil::format(
        "$0.$1.sst",
        snap->key.toString(),
        Random::singleton()->hex64());

    snap->state.add_sstable_files(filename);
  } else {
    RAISE(kIllegalStateError);
  }

  auto filepath = FileUtil::joinPaths(snap->base_path, filename);

  for (const auto& r : records) {
    if (record_ids.count(r.record_id) == 0) {
      continue;
    }
  }

  snap->nrecs += record_ids.size();
  snap->writeToDisk();
  *head_ = snap;

  return record_ids;
}

} // namespace tdsb
