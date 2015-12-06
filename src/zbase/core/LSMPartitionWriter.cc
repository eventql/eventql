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
#include <zbase/core/RecordVersionMap.h>
#include <stx/protobuf/msg.h>
#include <stx/logging.h>
#include <stx/wallclock.h>
#include <stx/logging.h>
#include <sstable/SSTableWriter.h>
#include <stx/protobuf/MessageDecoder.h>
#include <cstable/RecordShredder.h>
#include <cstable/CSTableWriter.h>

using namespace stx;

namespace zbase {

LSMPartitionWriter::LSMPartitionWriter(
    RefPtr<Partition> partition,
    PartitionSnapshotRef* head) :
    PartitionWriter(head),
    partition_(partition),
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

  HashMap<SHA1Hash, uint64_t> rec_versions;
  for (const auto& r : records) {
    if (snap->head_arena->fetchRecordVersion(r.record_id) < r.record_version) {
      rec_versions.emplace(r.record_id, 0);
    }
  }

  if (snap->compacting_arena.get() != nullptr) {
    for (auto& r : rec_versions) {
      auto v = snap->compacting_arena->fetchRecordVersion(r.first);
      if (v > r.second) {
        r.second = v;
      }
    }
  }

  const auto& tables = snap->state.lsm_tables();
  for (auto tbl = tables.rbegin(); tbl != tables.rend(); ++tbl) {
    RecordVersionMap::lookup(
        &rec_versions,
        FileUtil::joinPaths(snap->base_path, tbl->filename() + ".idx"));

    // FIMXE early exit...
  }

  Set<SHA1Hash> inserted_ids;
  if (!rec_versions.empty()) {
    for (auto r : records) {
      auto headv = rec_versions[r.record_id];
      if (headv > 0) {
        r.is_update = true;
      }

      if (r.record_version <= headv) {
        continue;
      }

      if (snap->head_arena->insertRecord(r)) {
        inserted_ids.emplace(r.record_id);
      }
    }
  }

  return inserted_ids;
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

  // flip arenas if records pending
  auto snap = head_->getSnapshot()->clone();
  if (snap->compacting_arena.get() == nullptr) {
    ScopedLock<std::mutex> write_lk(mutex_);
    if (snap->head_arena->size() > 0) {
      snap->compacting_arena = snap->head_arena;
      snap->head_arena = mkRef(new RecordArena());
      head_->setSnapshot(snap);
      snap = snap->clone();
    }
  }

  // flush arena to disk if pending
  if (snap->compacting_arena.get() != nullptr) {
    writeArenaToDisk(snap);
  }

  // commit compaction
  {
    ScopedLock<std::mutex> write_lk(mutex_);
    snap->writeToDisk();
    head_->setSnapshot(snap);
  }
}

void LSMPartitionWriter::writeArenaToDisk(RefPtr<PartitionSnapshot> snap) {
  auto t0 = WallClock::unixMicros();
  auto schema = partition_->getTable()->schema();
  auto arena = snap->compacting_arena;
  auto filename = Random::singleton()->hex64();
  auto filepath = FileUtil::joinPaths(snap->base_path, filename);

  {
    HashMap<SHA1Hash, uint64_t> vmap;
    auto cstable_schema = cstable::TableSchema::fromProtobuf(*schema);
    auto cstable_schema_ext = cstable_schema;
    cstable_schema_ext.addBool("__lsm_is_update", false);
    cstable_schema_ext.addString("__lsm_id", false);
    cstable_schema_ext.addUnsignedInteger("__lsm_version", false);

    auto cstable = cstable::CSTableWriter::createFile(
        filepath + ".cst",
        cstable::BinaryFormatVersion::v0_1_0,
        cstable_schema_ext);

    cstable::RecordShredder shredder(cstable.get(), &cstable_schema);
    auto is_update_col = cstable->getColumnWriter("__lsm_is_update");
    auto id_col = cstable->getColumnWriter("__lsm_id");
    auto version_col = cstable->getColumnWriter("__lsm_version");

    arena->fetchRecords([&] (const RecordRef& r) {
      msg::MessageObject obj;
      msg::MessageDecoder::decode(r.record, *schema, &obj);
      shredder.addRecordFromProtobuf(obj, *schema);
      is_update_col->writeBoolean(0, 0, r.is_update);
      id_col->writeString(0, 0, r.record_id.toString());
      version_col->writeUnsignedInt(0, 0, r.record_version);
      vmap.emplace(r.record_id, r.record_version);
    });

    cstable->commit();
    RecordVersionMap::write(vmap, filepath + ".idx");
  }

  snap->compacting_arena = nullptr;
  auto tblref = snap->state.add_lsm_tables();
  tblref->set_filename(filename);

  auto t1 = WallClock::unixMicros();

  stx::logDebug(
      "z1.core",
      "Writing arena with $0 records to disk for partition $1/$2/$3, took $4s",
      arena->size(),
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      snap->key.toString(),
      (double) (t1 - t0) / 1000000.0f);
}

} // namespace tdsb
