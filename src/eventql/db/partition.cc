/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include "eventql/eventql.h"
#include <eventql/db/partition.h>
#include <eventql/db/partition_map.h>
#include <eventql/db/partition_change_notification.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/uri.h>
#include <eventql/util/logging.h>
#include <eventql/util/random.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/wallclock.h>
#include <eventql/util/protobuf/MessageEncoder.h>
#include <eventql/util/protobuf/MessageDecoder.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/io/sstable/sstablereader.h>
#include <eventql/db/partition_reader.h>
#include <eventql/db/partition_writer.h>
#include <eventql/db/partition_replication.h>
#include <eventql/db/file_tracker.h>
#include <eventql/server/server_stats.h>

namespace eventql {

RefPtr<Partition> Partition::create(
    const String& tsdb_namespace,
    RefPtr<Table> table,
    const SHA1Hash& partition_key,
    const PartitionDiscoveryResponse& discovery_info,
    DatabaseContext* dbctx) {
  logDebug(
      "tsdb",
      "Creating new partition; stream='$0' partition='$1'",
      table->name(),
      partition_key.toString());

  auto pdir_rel = StringUtil::format(
      "$0/$1/$2",
      tsdb_namespace,
      SHA1::compute(table->name()).toString(),
      partition_key.toString());

  auto pdir = FileUtil::joinPaths(dbctx->db_path, pdir_rel);

  FileUtil::mkdir_p(pdir);

  auto uuid = Random::singleton()->sha1();

  PartitionState state;
  state.set_tsdb_namespace(tsdb_namespace);
  state.set_partition_key(partition_key.data(), partition_key.size());
  state.set_table_key(table->name());
  state.set_uuid(uuid.data(), uuid.size());
  state.set_partition_keyrange_begin(discovery_info.keyrange_begin());
  state.set_partition_keyrange_end(discovery_info.keyrange_end());
  state.set_table_generation(table->config().generation());

  auto snap = mkRef(
    new PartitionSnapshot(table.get(), state, pdir, pdir_rel, dbctx, 0));
  snap->writeToDisk();

  auto partition = mkRef(new Partition(partition_key, dbctx, snap, table));

  {
    auto rc = partition->getWriter()->applyMetadataChange(discovery_info);
    if (!rc.isSuccess()) {
      RAISE(kRuntimeError, rc.message());
    }
  }

  return partition;
}

RefPtr<Partition> Partition::reopen(
    const String& tsdb_namespace,
    RefPtr<Table> table,
    const SHA1Hash& partition_key,
    DatabaseContext* dbctx) {
  auto pdir_rel = StringUtil::format(
      "$0/$1/$2",
      tsdb_namespace,
      SHA1::compute(table->name()).toString(),
      partition_key.toString());

  auto pdir = FileUtil::joinPaths(dbctx->db_path, pdir_rel);

  auto state = msg::decode<PartitionState>(
      FileUtil::read(FileUtil::joinPaths(pdir, "_snapshot")));

  auto nrecs = 0;
  const auto& files = state.sstable_files();
  for (const auto& f : files) {
    auto is = FileInputStream::openFile(FileUtil::joinPaths(pdir, f));
    auto header = sstable::FileHeaderReader::readMetaPage(is.get());
    nrecs += header.rowCount();
  }

  logTrace(
      "tsdb",
      "Loading partition $0/$1/$2 ($3 records)",
      tsdb_namespace,
      table->name(),
      partition_key.toString(),
      nrecs);

  auto snap = mkRef(
      new PartitionSnapshot(table.get(), state, pdir, pdir_rel, dbctx, nrecs));
  return new Partition(partition_key, dbctx, snap, table);
}

Partition::Partition(
    SHA1Hash partition_id,
    DatabaseContext* dbctx,
    RefPtr<PartitionSnapshot> head,
    RefPtr<Table> table) :
    partition_id_(partition_id),
    dbctx_(dbctx),
    head_(head),
    table_(table) {
  evqld_stats()->num_partitions_opened.incr(1);
}

Partition::~Partition() {
  evqld_stats()->num_partitions_opened.decr(1);
}

SHA1Hash Partition::uuid() const {
  auto snap = head_.getSnapshot();
  return snap->uuid();
}

RefPtr<PartitionWriter> Partition::getWriter() {
  std::unique_lock<std::mutex> lk(writer_lock_);
  if (writer_.get() == nullptr) {
    writer_ = mkRef<PartitionWriter>(new LSMPartitionWriter(dbctx_, this, &head_));
  }

  return writer_;
}

RefPtr<PartitionReader> Partition::getReader() {
  return new LSMPartitionReader(table_, head_.getSnapshot());
}

RefPtr<PartitionSnapshot> Partition::getSnapshot() {
  return head_.getSnapshot();
}

RefPtr<Table> Partition::getTable() {
  return table_;
}

PartitionInfo Partition::getInfo() const {
  auto snap = head_.getSnapshot();

  auto checksum = SHA1::compute(
      StringUtil::format("$0~$1", snap->key.toString(), snap->nrecs));

  PartitionInfo pi;
  pi.set_partition_key(snap->key.toString());
  pi.set_table_name(table_->name());
  pi.set_checksum(checksum.toString());
  pi.set_cstable_version(snap->state.cstable_version());
  pi.set_exists(true);
  return pi;
}

RefPtr<PartitionReplication> Partition::getReplicationStrategy() {
  return new LSMPartitionReplication(this, dbctx_);
}

bool Partition::upgradeToLSMv2() const {
  return true;
}

String Partition::getRelativePath() const {
  return head_.getSnapshot()->rel_path;
}

String Partition::getAbsolutePath() const {
  return head_.getSnapshot()->base_path;
}

MetadataTransaction Partition::getLastMetadataTransaction() const {
  auto snap = head_.getSnapshot();

  if (snap->state.last_metadata_txnid().empty()) {
    return MetadataTransaction(SHA1Hash(), 0);
  }

  return MetadataTransaction(
      SHA1Hash(
          snap->state.last_metadata_txnid().data(),
          snap->state.last_metadata_txnid().size()),
      snap->state.last_metadata_txnseq());
}

size_t Partition::getTotalDiskSize() const {
  if (table_->storage() == eventql::TBL_STORAGE_STATIC) {
    return 0;
  }

  auto snap = head_.getSnapshot();
  size_t size = 0;
  for (const auto& tbl : snap->state.lsm_tables()) {
    size += tbl.size_bytes();
  }

  return size;
}

bool Partition::isSplitting() const {
  auto snap = head_.getSnapshot();
  if (snap->state.is_splitting()) {
    return true;
  }

  Set<String> keys;
  for (const auto& t : snap->state.replication_targets()){
    keys.insert(t.keyrange_begin());
    keys.insert(t.keyrange_end());
  }

  if (keys.size() > 2) {
    return true;
  }

  return false;
}

LazyPartition::LazyPartition() {
  evqld_stats()->num_partitions.incr(1);
}

LazyPartition::LazyPartition(
    RefPtr<Partition> partition) :
    partition_(partition) {
  evqld_stats()->num_partitions.incr(1);
}

LazyPartition::~LazyPartition() {
  evqld_stats()->num_partitions.decr(1);
}

RefPtr<Partition> LazyPartition::getPartition(
    const String& tsdb_namespace,
    RefPtr<Table> table,
    const SHA1Hash& partition_key,
    DatabaseContext* dbctx,
    PartitionMap* pmap) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (partition_.get() != nullptr) {
    auto partition = partition_;
    return partition;
  }

  partition_ = Partition::reopen(
      tsdb_namespace,
      table,
      partition_key,
      dbctx);

  auto partition = partition_;
  lk.unlock();

  auto change = mkRef(new PartitionChangeNotification());
  change->partition = partition;
  pmap->publishPartitionChange(change);

  return partition;
}

RefPtr<Partition> LazyPartition::getPartition() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (partition_.get() == nullptr) {
    RAISE(kRuntimeError, "partition not loaded");
  }

  auto partition = partition_;
  return partition;
}

bool LazyPartition::isLoaded() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return partition_.get() != nullptr;
}

}
