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
#include <sstable/sstablereader.h>

using namespace stx;

namespace zbase {

LSMPartitionWriter::LSMPartitionWriter(
    RefPtr<Partition> partition,
    PartitionSnapshotRef* head) :
    PartitionWriter(head),
    partition_(partition),
    compaction_strategy_(
        new SimpleCompactionStrategy(
            partition_->getTable(),
            head->getSnapshot()->base_path)),
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

bool LSMPartitionWriter::needsCommit() {
  ScopedLock<std::mutex> write_lk(mutex_);
  return head_->getSnapshot()->head_arena->size() > 0;
}

bool LSMPartitionWriter::needsCompaction() {
  if (needsCommit() || needsUpgradeFromV1()) {
    return true;
  }

  auto snap = head_->getSnapshot();
  return compaction_strategy_->needsCompaction(
      Vector<LSMTableRef>(
          snap->state.lsm_tables().begin(),
          snap->state.lsm_tables().end()));
}

bool LSMPartitionWriter::commit() {
  ScopedLock<std::mutex> commit_lk(commit_mutex_);
  RefPtr<RecordArena> arena;

  // flip arenas if records pending
  {
    ScopedLock<std::mutex> write_lk(mutex_);
    auto snap = head_->getSnapshot()->clone();
    if (snap->compacting_arena.get() == nullptr &&
        snap->head_arena->size() > 0) {
      snap->compacting_arena = snap->head_arena;
      snap->head_arena = mkRef(new RecordArena());
      head_->setSnapshot(snap);
    }
    arena = snap->compacting_arena;
  }

  // flush arena to disk if pending
  if (arena.get() && arena->size() > 0) {
    auto snap = head_->getSnapshot();
    auto filename = Random::singleton()->hex64();
    auto filepath = FileUtil::joinPaths(snap->base_path, filename);
    auto t0 = WallClock::unixMicros();
    writeArenaToDisk(arena, snap->state.lsm_sequence() + 1, filepath);
    auto t1 = WallClock::unixMicros();

    stx::logDebug(
        "z1.core",
        "Committing partition $1/$2/$3 ($0 records), took $4s",
        arena->size(),
        snap->state.tsdb_namespace(),
        snap->state.table_key(),
        snap->key.toString(),
        (double) (t1 - t0) / 1000000.0f);

    ScopedLock<std::mutex> write_lk(mutex_);
    snap = head_->getSnapshot()->clone();
    auto tblref = snap->state.add_lsm_tables();
    tblref->set_filename(filename);
    tblref->set_first_sequence(snap->state.lsm_sequence() + 1);
    tblref->set_last_sequence(snap->state.lsm_sequence() + arena->size());
    snap->state.set_lsm_sequence(snap->state.lsm_sequence() + arena->size());
    snap->compacting_arena = nullptr;
    snap->writeToDisk();
    head_->setSnapshot(snap);
    return true;
  } else {
    return false;
  }
}

bool LSMPartitionWriter::compact() {
  if (needsUpgradeFromV1()) {
    upgradeFromV1();
  }

  auto dirty = commit();

  // fetch current table list
  auto snap = head_->getSnapshot()->clone();

  Vector<LSMTableRef> new_tables;
  Vector<LSMTableRef> old_tables(
      snap->state.lsm_tables().begin(),
      snap->state.lsm_tables().end());

  // compact
  auto t0 = WallClock::unixMicros();
  if (!compaction_strategy_->compact(old_tables, &new_tables)) {
    return dirty;
  }
  auto t1 = WallClock::unixMicros();

  stx::logDebug(
      "z1.core",
      "Compacting partition $0/$1/$2, took $3s",
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      snap->key.toString(),
      (double) (t1 - t0) / 1000000.0f);

  // commit table list
  ScopedLock<std::mutex> write_lk(mutex_);
  snap = head_->getSnapshot()->clone();

  if (snap->state.lsm_tables().size() < old_tables.size()) {
    RAISE(kConcurrentModificationError, "concurrent compaction");
  }

  size_t i = 0;
  for (const auto& tbl : snap->state.lsm_tables()) {
    if (i < old_tables.size()) {
      if (old_tables[i].filename() != tbl.filename()) {
        RAISE(kConcurrentModificationError, "concurrent compaction");
      }
    } else {
      new_tables.push_back(tbl);
    }

    ++i;
  }

  snap->state.mutable_lsm_tables()->Clear();
  for (const auto& tbl :  new_tables) {
    *snap->state.add_lsm_tables() = tbl;
  }

  snap->writeToDisk();
  head_->setSnapshot(snap);
  write_lk.unlock();

  Set<String> delete_filenames;
  for (const auto& tbl : old_tables) {
    delete_filenames.emplace(tbl.filename());
  }
  for (const auto& tbl : new_tables) {
    delete_filenames.erase(tbl.filename());
  }

  for (const auto& f : delete_filenames) {
    // FIXME: delayed delete
    FileUtil::rm(FileUtil::joinPaths(snap->base_path, f + ".cst"));
    FileUtil::rm(FileUtil::joinPaths(snap->base_path, f + ".idx"));
  }

  return true;
}

void LSMPartitionWriter::writeArenaToDisk(
      RefPtr<RecordArena> arena,
      uint64_t sequence,
      const String& filename) {
  auto schema = partition_->getTable()->schema();

  {
    OrderedMap<SHA1Hash, uint64_t> vmap;
    auto cstable_schema = cstable::TableSchema::fromProtobuf(*schema);
    auto cstable_schema_ext = cstable_schema;
    cstable_schema_ext.addBool("__lsm_is_update", false);
    cstable_schema_ext.addString("__lsm_id", false);
    cstable_schema_ext.addUnsignedInteger("__lsm_version", false);
    cstable_schema_ext.addUnsignedInteger("__lsm_sequence", false);

    auto cstable = cstable::CSTableWriter::createFile(
        filename + ".cst",
        cstable::BinaryFormatVersion::v0_1_0,
        cstable_schema_ext);

    cstable::RecordShredder shredder(cstable.get(), &cstable_schema);
    auto is_update_col = cstable->getColumnWriter("__lsm_is_update");
    auto id_col = cstable->getColumnWriter("__lsm_id");
    auto version_col = cstable->getColumnWriter("__lsm_version");
    auto sequence_col = cstable->getColumnWriter("__lsm_sequence");

    arena->fetchRecords([&] (const RecordRef& r) {
      msg::MessageObject obj;
      msg::MessageDecoder::decode(r.record, *schema, &obj);
      shredder.addRecordFromProtobuf(obj, *schema);
      is_update_col->writeBoolean(0, 0, r.is_update);
      String id_str((const char*) r.record_id.data(), r.record_id.size());
      id_col->writeString(0, 0, id_str);
      version_col->writeUnsignedInt(0, 0, r.record_version);
      sequence_col->writeUnsignedInt(0, 0, sequence++);
      vmap.emplace(r.record_id, r.record_version);
    });

    cstable->commit();
    RecordVersionMap::write(vmap, filename + ".idx");
  }
}

ReplicationState LSMPartitionWriter::fetchReplicationState() const {
  auto snap = head_->getSnapshot();
  auto repl_state = snap->state.replication_state();
  String tbl_uuid((char*) snap->uuid().data(), snap->uuid().size());

  if (repl_state.uuid() == tbl_uuid) {
    return repl_state;
  } else {
    ReplicationState state;
    state.set_uuid(tbl_uuid);
    return state;
  }
}

void LSMPartitionWriter::commitReplicationState(const ReplicationState& state) {
  ScopedLock<std::mutex> write_lk(mutex_);
  auto snap = head_->getSnapshot()->clone();
  *snap->state.mutable_replication_state() = state;
  snap->writeToDisk();
  head_->setSnapshot(snap);
}

void LSMPartitionWriter::upgradeFromV1() {
  ScopedLock<std::mutex> upgrade_lk(upgrade_mutex_);
  auto snap = head_->getSnapshot();

  logNotice(
      "z1.core",
      "Upgrading partition $0/$1/$2 to LSM storage v0.2.0",
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      snap->key.toString());

  auto cst_filepath = FileUtil::joinPaths(snap->base_path, "_cstable");
  if (FileUtil::exists(cst_filepath)) {
    FileUtil::rm(cst_filepath);
  }

  auto cstm_filepath = FileUtil::joinPaths(snap->base_path, "_cstable_state");
  if (FileUtil::exists(cstm_filepath)) {
    FileUtil::rm(cstm_filepath);
  }

  auto files = snap->state.sstable_files();
  size_t nrecs = 0;
  for (const auto& f : files) {
    auto fpath = FileUtil::joinPaths(snap->base_path, f);
    sstable::SSTableReader reader(fpath);

    Vector<RecordRef> records;
    auto cursor = reader.getCursor();
    while (cursor->valid()) {
      void* key;
      size_t key_size;
      cursor->getKey(&key, &key_size);
      if (key_size != SHA1Hash::kSize) {
        RAISE(kRuntimeError, "invalid row");
      }

      void* data;
      size_t data_size;
      cursor->getData(&data, &data_size);

      ++nrecs;
      auto record_id = SHA1Hash(key, key_size);
      records.emplace_back(record_id, 1, Buffer(data, data_size));

      if (records.size() == 8192 * 8) {
        insertRecords(records);
        records.clear();
        commit();
      }

      if (!cursor->next()) {
        break;
      }
    }

    insertRecords(records);
  }

  commit();

  {
    ScopedLock<std::mutex> write_lk(mutex_);
    auto snap = head_->getSnapshot()->clone();
    snap->state.mutable_sstable_files()->Clear();
    snap->writeToDisk();
    head_->setSnapshot(snap);
  }

  for (const auto& f : files) {
    auto fpath = FileUtil::joinPaths(snap->base_path, f);
    FileUtil::mv(fpath, fpath + ".DELETED_BY_UPGRADE");
  }

  auto idset_filepath = FileUtil::joinPaths(snap->base_path, "_idset");
  if (FileUtil::exists(idset_filepath)) {
    FileUtil::mv(idset_filepath, idset_filepath + ".DELETED_BY_UPGRADE");
  }

  auto repl_filepath = FileUtil::joinPaths(snap->base_path, "_repl");
  if (FileUtil::exists(repl_filepath)) {
    FileUtil::rm(repl_filepath);
  }

  logNotice(
      "z1.core",
      "Upgrade for partition $1/$2/$3 ($0 records) to LSM storage v0.2.0 done",
      nrecs,
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      snap->key.toString());
}

bool LSMPartitionWriter::needsUpgradeFromV1() {
  auto snap = head_->getSnapshot();
  return
      snap->state.sstable_files().size() > 0 ||
      FileUtil::exists(FileUtil::joinPaths(snap->base_path, "_idset")) ||
      FileUtil::exists(FileUtil::joinPaths(snap->base_path, "_repl")) ||
      FileUtil::exists(FileUtil::joinPaths(snap->base_path, "_cstable_state")) ||
      FileUtil::exists(FileUtil::joinPaths(snap->base_path, "_cstable"));
}

} // namespace tdsb
