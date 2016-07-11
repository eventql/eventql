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
#include <eventql/db/Partition.h>
#include <eventql/db/LSMPartitionWriter.h>
#include <eventql/db/LSMTableIndex.h>
#include <eventql/db/LSMPartitionReader.h>
#include <eventql/db/metadata_operation.h>
#include <eventql/db/metadata_coordinator.h>
#include <eventql/db/server_allocator.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/util/logging.h>
#include <eventql/util/wallclock.h>
#include <eventql/util/logging.h>
#include <eventql/util/protobuf/MessageDecoder.h>
#include <eventql/io/cstable/RecordShredder.h>
#include <eventql/io/cstable/cstable_writer.h>

#include "eventql/eventql.h"

namespace eventql {

LSMPartitionWriter::LSMPartitionWriter(
    ServerCfg* cfg,
    RefPtr<Partition> partition,
    PartitionSnapshotRef* head) :
    PartitionWriter(head),
    partition_(partition),
    compaction_strategy_(
        new SimpleCompactionStrategy(
            partition_,
            cfg->idx_cache.get())),
    idx_cache_(cfg->idx_cache.get()),
    cdir_(cfg->config_directory),
    repl_(cfg->repl_scheme.get()),
    partition_split_threshold_(kDefaultPartitionSplitThresholdBytes) {}

Set<SHA1Hash> LSMPartitionWriter::insertRecords(const Vector<RecordRef>& records) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (frozen_) {
    RAISE(kIllegalStateError, "partition is frozen");
  }

  auto snap = head_->getSnapshot();

  logTrace(
      "tsdb",
      "Insert $0 record into partition $1/$2/$3",
      records.size(),
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      snap->key.toString());

  HashMap<SHA1Hash, uint64_t> rec_versions;
  for (const auto& r : records) {
    rec_versions.emplace(r.record_id, snap->head_arena->fetchRecordVersion(r.record_id));
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
  if (tables.size() > kMaxLSMTables) {
    RAISE(kRuntimeError, "partition is overloaded, can't insert");
  }

  for (auto tbl = tables.rbegin(); tbl != tables.rend(); ++tbl) {
    auto idx = idx_cache_->lookup(
        FileUtil::joinPaths(snap->rel_path, tbl->filename()));
    idx->lookup(&rec_versions);

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

  lk.unlock();

  if (needsUrgentCommit()) {
    commit();
  }

  if (needsUrgentCompaction()) {
    compact();
  }

  return inserted_ids;
}

bool LSMPartitionWriter::needsCommit() {
  return head_->getSnapshot()->head_arena->size() > 0;
}

bool LSMPartitionWriter::needsUrgentCommit() {
  return head_->getSnapshot()->head_arena->size() > kMaxArenaRecords;
}

bool LSMPartitionWriter::needsCompaction() {
  if (needsCommit()) {
    return true;
  }

  auto snap = head_->getSnapshot();
  return compaction_strategy_->needsCompaction(
      Vector<LSMTableRef>(
          snap->state.lsm_tables().begin(),
          snap->state.lsm_tables().end()));
}

bool LSMPartitionWriter::needsUrgentCompaction() {
  auto snap = head_->getSnapshot();
  return compaction_strategy_->needsUrgentCompaction(
      Vector<LSMTableRef>(
          snap->state.lsm_tables().begin(),
          snap->state.lsm_tables().end()));
}

bool LSMPartitionWriter::commit() {
  ScopedLock<std::mutex> commit_lk(commit_mutex_);
  RefPtr<PartitionArena> arena;

  // flip arenas if records pending
  {
    ScopedLock<std::mutex> write_lk(mutex_);
    auto snap = head_->getSnapshot()->clone();
    if (snap->compacting_arena.get() == nullptr &&
        snap->head_arena->size() > 0) {
      snap->compacting_arena = snap->head_arena;
      snap->head_arena = mkRef(
          new PartitionArena(*partition_->getTable()->schema()));
      head_->setSnapshot(snap);
    }
    arena = snap->compacting_arena;
  }

  // flush arena to disk if pending
  bool commited = false;
  if (arena.get() && arena->size() > 0) {
    auto snap = head_->getSnapshot();
    auto filename = Random::singleton()->hex64();
    auto filepath = FileUtil::joinPaths(snap->base_path, filename);
    auto t0 = WallClock::unixMicros();
    arena->writeToDisk(filepath, snap->state.lsm_sequence() + 1);
    auto t1 = WallClock::unixMicros();

    logDebug(
        "z1.core",
        "Committing partition $3/$4/$5 (num_records=$0, sequence=$1..$2), took $6s",
        arena->size(),
        snap->state.lsm_sequence() + 1,
        snap->state.lsm_sequence() + arena->size(),
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
    tblref->set_size_bytes(FileUtil::size(filepath + ".cst"));
    tblref->set_has_skiplist(true);
    snap->state.set_lsm_sequence(snap->state.lsm_sequence() + arena->size());
    snap->compacting_arena = nullptr;
    snap->writeToDisk();
    head_->setSnapshot(snap);
    commited = true;
  }

  commit_lk.unlock();

  if (needsSplit()) {
    auto rc = split();
    if (!rc.isSuccess()) {
      logWarning("evqld", "partition split failed: $0", rc.message());
    }
  }

  return commited;
}

bool LSMPartitionWriter::compact() {
  ScopedLock<std::mutex> compact_lk(compaction_mutex_, std::defer_lock);
  if (!compact_lk.try_lock()) {
    return false;
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

  logDebug(
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

  // delete
  Set<String> delete_filenames;
  for (const auto& tbl : old_tables) {
    delete_filenames.emplace(tbl.filename());
  }
  for (const auto& tbl : new_tables) {
    delete_filenames.erase(tbl.filename());
  }

  compact_lk.unlock();

  for (const auto& f : delete_filenames) {
    auto trash_dir = FileUtil::joinPaths(snap->base_path, "../../../../../trash");
    {
      auto trash_link = File::openFile(
          FileUtil::joinPaths(trash_dir, Random::singleton()->hex128() + ".trash"),
          File::O_WRITE | File::O_CREATE);

      trash_link.write(
          FileUtil::joinPaths(snap->base_path, f + ".cst") + "\n" +
          FileUtil::joinPaths(snap->base_path, f + ".idx") + "\n");
    }

    idx_cache_->flush(FileUtil::joinPaths(snap->rel_path, f));
  }

  // maybe split this partition
  if (needsSplit()) {
    auto rc = split();
    if (!rc.isSuccess()) {
      logWarning("evqld", "partition split failed: $0", rc.message());
    }
  }

  return true;
}

bool LSMPartitionWriter::needsSplit() const {
  auto snap = head_->getSnapshot();
  if (snap->state.lifecycle_state() != PDISCOVERY_SERVE) {
    return false;
  }

  size_t size = 0;
  for (const auto& tbl : snap->state.lsm_tables()) {
    size += tbl.size_bytes();
  }

  return size > partition_split_threshold_;
}

Status LSMPartitionWriter::split() {
  ScopedLock<std::mutex> split_lk(split_mutex_, std::defer_lock);
  if (!split_lk.try_lock()) {
    return Status(eConcurrentModificationError, "split is already running");
  }

  auto snap = head_->getSnapshot();
  auto table = partition_->getTable();
  auto keyspace = table->getKeyspaceType();

  if (snap->state.lifecycle_state() != PDISCOVERY_SERVE) {
    return Status(eIllegalArgumentError, "can't split non-serving partition");
  }

  String midpoint;
  {
    auto cmp = [keyspace] (const String& a, const String& b) -> bool {
      return comparePartitionKeys(
          keyspace,
          encodePartitionKey(keyspace, a),
          encodePartitionKey(keyspace, b)) < 0;
    };

    LSMPartitionReader reader(table, snap);
    String minval;
    String maxval;
    auto rc = reader.findMedianValue(
        table->getPartitionKey(),
        cmp,
        &minval,
        &midpoint,
        &maxval);

    if (!rc.isSuccess()) {
      return rc;
    }

    if (minval == midpoint || maxval == midpoint) {
      return Status(eRuntimeError, "no suitable split point found");
    }
  }

  logInfo(
      "z1.replication",
      "Splitting partition $0/$1/$2 at '$3'",
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      snap->key.toString(),
      midpoint);

  auto split_partition_id_low = Random::singleton()->sha1();
  auto split_partition_id_high = Random::singleton()->sha1();

  SplitPartitionOperation op;
  op.set_partition_id(snap->key.data(), snap->key.size());
  op.set_split_point(encodePartitionKey(keyspace, midpoint));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(Random::singleton()->random64());

  ServerAllocator server_alloc(cdir_);

  Set<String> split_servers_low;
  {
    auto rc = server_alloc.allocateServers(3, &split_servers_low);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  for (const auto& s : split_servers_low) {
    op.add_split_servers_low(s);
  }

  Set<String> split_servers_high;
  {
    auto rc = server_alloc.allocateServers(3, &split_servers_high);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  for (const auto& s : split_servers_high) {
    op.add_split_servers_high(s);
  }

  auto table_config = cdir_->getTableConfig(
      snap->state.tsdb_namespace(),
      snap->state.table_key());
  MetadataOperation envelope(
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      METAOP_SPLIT_PARTITION,
      SHA1Hash(
          table_config.metadata_txnid().data(),
          table_config.metadata_txnid().size()),
      Random::singleton()->sha1(),
      *msg::encode(op));

  MetadataCoordinator coordinator(cdir_);
  return coordinator.performAndCommitOperation(
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      envelope);
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

Status LSMPartitionWriter::applyMetadataChange(
    const PartitionDiscoveryResponse& discovery_info) {
  ScopedLock<std::mutex> write_lk(mutex_);
  auto snap = head_->getSnapshot()->clone();

  logTrace(
      "evqld",
      "Applying metadata change to partition $0/$1/$2: $3",
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      snap->key.toString(),
      discovery_info.DebugString());

  if (snap->state.last_metadata_txnseq() >= discovery_info.txnseq()) {
    return Status(eConcurrentModificationError, "version conflict");
  }

  snap->state.set_last_metadata_txnid(discovery_info.txnid());
  snap->state.set_last_metadata_txnseq(discovery_info.txnseq());
  snap->state.set_lifecycle_state(discovery_info.code());
  snap->state.set_is_splitting(discovery_info.is_splitting());

  // backfill keyrange
  if (snap->state.partition_keyrange_end().size() == 0 &&
      discovery_info.keyrange_end().size() > 0) {
    snap->state.set_partition_keyrange_end(discovery_info.keyrange_end());
  }

  snap->state.mutable_split_partition_ids()->Clear();
  for (const auto& p : discovery_info.split_partition_ids()) {
    snap->state.add_split_partition_ids(p);
  }

  snap->state.set_has_joining_servers(false);
  snap->state.mutable_replication_targets()->Clear();
  for (const auto& dt : discovery_info.replication_targets()) {
    auto pt = snap->state.add_replication_targets();
    pt->set_server_id(dt.server_id());
    pt->set_placement_id(dt.placement_id());
    pt->set_partition_id(dt.partition_id());
    pt->set_keyrange_begin(dt.keyrange_begin());
    pt->set_keyrange_end(dt.keyrange_end());

    // backfill -- remove after rollout
    try {
      auto replicas = repl_->replicasFor(
          SHA1Hash(dt.partition_id().data(), dt.partition_id().size()));

      for (const auto& r : replicas) {
        if (r.name == dt.server_id()) {
          pt->set_legacy_token(r.unique_id.toString());
          break;
        }
      }
    } catch (...) {}
    // eof backfill

    if (dt.is_joining()) {
      pt->set_is_joining(true);
      snap->state.set_has_joining_servers(true);
    }
  }

  snap->writeToDisk();
  head_->setSnapshot(snap);

  return Status::success();
}

} // namespace tdsb
