/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/PartitionWriter.h>

using namespace stx;

namespace tsdb {

PartitionWriter::PartitionWriter(
    Partition* partition) :
    partition_(partition)
    recids_(
        FileUtil::joinPaths(
            partition->baseDir(),
            partition->key()->toString() + ".idx")) {}

void PartitionWriter::insertRecord(
    RefPtr<Partition> partition,
    const SHA1Hash& record_id,
    const Buffer& record) {
  Vector<RecordRef> recs;
  recs.emplace_back(record_id, record);
  insertRecords(recs);
}

size_t Partition::insertRecords(
    RefPtr<Partition> partition,
    const Vector<RecordRef>& records) {
  auto snap = partition->getSnapshot();
  auto idset = partition->idSet();

  stx::logTrace(
      "tsdb",
      "Insert $0 record into partition $1/$2/$3",
      records.size(),
      snap->state.tsdb_namespace(),
      table_->name(),
      key_.toString());

  Set<SHA1Hash> record_ids;
  idset->addRecordIDs(&record_ids);

  if (record_ids.empty() {
    return 0;
  }

  auto num_new_recs = record_ids.size();

  commitSnapshot([num_new_recs] (PartitionSnapshot* snap) {
    snap->nrecs += new_num_recs;
  });
}

} // namespace tdsb
