/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <eventql/util/io/fileutil.h>
#include <eventql/core/Partition.h>
#include <eventql/core/LogPartitionWriter.h>
#include <eventql/core/LogPartitionReader.h>
#include <eventql/core/LogPartitionCompactionState.pb.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/util/logging.h>
#include <eventql/util/wallclock.h>
#include <eventql/infra/sstable/SSTableWriter.h>
#include <eventql/util/protobuf/MessageDecoder.h>
#include <eventql/infra/cstable/RecordShredder.h>
#include <eventql/infra/cstable/CSTableWriter.h>

#include "eventql/eventql.h"

namespace eventql {

LogPartitionWriter::LogPartitionWriter(
    RefPtr<Partition> partition,
    PartitionSnapshotRef* head) :
    PartitionWriter(head),
    partition_(partition),
    idset_(FileUtil::joinPaths(head_->getSnapshot()->base_path, "_idset")),
    max_datafile_size_(kDefaultMaxDatafileSize) {}

Set<SHA1Hash> LogPartitionWriter::insertRecords(const Vector<RecordRef>& records) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (frozen_) {
    RAISE(kIllegalStateError, "partition is frozen");
  }

  auto snap = head_->getSnapshot()->clone();

  logTrace(
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

bool LogPartitionWriter::needsCompaction() {
  auto snap = head_->getSnapshot();
  String tbl_uuid((char*) snap->uuid().data(), snap->uuid().size());

  auto metapath = FileUtil::joinPaths(snap->base_path, "_cstable_state");

  if (FileUtil::exists(metapath)) {
    auto metadata = msg::decode<LogPartitionCompactionState>(
        FileUtil::read(metapath));

    if (metadata.uuid() == tbl_uuid &&
        metadata.offset() >= snap->nrecs) {
      return false;
    }
  }

  return true;
}

bool LogPartitionWriter::commit() {
 return true; // noop
}

bool LogPartitionWriter::compact() {
  if (!needsCompaction()) {
    return false;
  }

  auto t0 = WallClock::unixMicros();
  auto snap = head_->getSnapshot();

  logDebug(
      "tsdb",
      "Building CSTable index for partition $0/$1/$2",
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      snap->key.toString());

  auto filepath = FileUtil::joinPaths(snap->base_path, "_cstable");
  auto filepath_tmp = filepath + "." + Random::singleton()->hex128();
  auto schema = partition_->getTable()->schema();

  {
    auto cstable = cstable::CSTableWriter::createFile(
        filepath_tmp,
        cstable::BinaryFormatVersion::v0_1_0,
        cstable::TableSchema::fromProtobuf(*schema));

    cstable::RecordShredder shredder(cstable.get());

    auto reader_ptr = partition_->getReader();
    auto& reader = dynamic_cast<LogPartitionReader&>(*reader_ptr);

    std::atomic<size_t> total_size(0);
    reader.fetchRecords([this, &schema, &shredder, &total_size, &snap] (const Buffer& record) {
      msg::MessageObject obj;
      msg::MessageDecoder::decode(record.data(), record.size(), *schema, &obj);
      shredder.addRecordFromProtobuf(obj, *schema);

      total_size += record.size();
      if (total_size > 1024llu * 1024llu * 1024llu * 4llu) {
        RAISEF(
            kRuntimeError,
            "CSTable is too large $0/$1/$2: $3",
            snap->state.tsdb_namespace(),
            snap->state.table_key(),
            snap->key.toString(),
            total_size.load());
      }
    });

    cstable->commit();
  }

  auto metapath = FileUtil::joinPaths(snap->base_path, "_cstable_state");
  auto metapath_tmp = metapath + "." + Random::singleton()->hex128();
  {
    auto metafile = File::openFile(
        metapath_tmp,
        File::O_CREATE | File::O_WRITE);

    LogPartitionCompactionState metadata;
    metadata.set_offset(snap->nrecs);
    metadata.set_uuid(snap->uuid().data(), snap->uuid().size());

    auto buf = msg::encode(metadata);
    metafile.write(buf->data(), buf->size());
  }

  FileUtil::mv(filepath_tmp, filepath);
  FileUtil::mv(metapath_tmp, metapath);

  auto t1 = WallClock::unixMicros();
  logDebug(
      "tsdb",
      "Commiting CSTable index for partition $0/$1/$2, building took $3s",
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      snap->key.toString(),
      (double) (t1 - t0) / 1000000.0f);

  return true;
}

} // namespace tdsb
