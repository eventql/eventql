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
#include <eventql/db/Partition.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/uri.h>
#include <eventql/util/logging.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/wallclock.h>
#include <eventql/util/protobuf/MessageEncoder.h>
#include <eventql/util/protobuf/MessageDecoder.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/io/sstable/sstablereader.h>
#include <eventql/db/LogPartitionReader.h>
#include <eventql/db/LogPartitionWriter.h>
#include <eventql/db/LogPartitionReplication.h>
#include <eventql/db/LSMPartitionReader.h>
#include <eventql/db/LSMPartitionWriter.h>
#include <eventql/db/LSMPartitionReplication.h>
#include <eventql/db/StaticPartitionReader.h>
#include <eventql/db/StaticPartitionWriter.h>
#include <eventql/db/StaticPartitionReplication.h>
#include <eventql/server/server_stats.h>

#include "eventql/eventql.h"

namespace eventql {

RefPtr<Partition> Partition::create(
    const String& tsdb_namespace,
    RefPtr<Table> table,
    const SHA1Hash& partition_key,
    const PartitionDiscoveryResponse& discovery_info,
    ServerCfg* cfg) {
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

  auto pdir = FileUtil::joinPaths(cfg->db_path, pdir_rel);

  FileUtil::mkdir_p(pdir);

  auto uuid = Random::singleton()->sha1();

  PartitionState state;
  state.set_tsdb_namespace(tsdb_namespace);
  state.set_partition_key(partition_key.data(), partition_key.size());
  state.set_table_key(table->name());
  state.set_uuid(uuid.data(), uuid.size());
  state.set_partition_keyrange_begin(discovery_info.keyrange_begin());
  state.set_partition_keyrange_end(discovery_info.keyrange_end());

  auto snap = mkRef(
    new PartitionSnapshot(table.get(), state, pdir, pdir_rel, 0));
  snap->writeToDisk();

  auto partition = mkRef(new Partition(partition_key, cfg, snap, table));

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
    ServerCfg* cfg) {
  auto pdir_rel = StringUtil::format(
      "$0/$1/$2",
      tsdb_namespace,
      SHA1::compute(table->name()).toString(),
      partition_key.toString());

  auto pdir = FileUtil::joinPaths(cfg->db_path, pdir_rel);

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
      new PartitionSnapshot(table.get(), state, pdir, pdir_rel, nrecs));
  return new Partition(partition_key, cfg, snap, table);
}

Partition::Partition(
    SHA1Hash partition_id,
    ServerCfg* cfg,
    RefPtr<PartitionSnapshot> head,
    RefPtr<Table> table) :
    partition_id_(partition_id),
    cfg_(cfg),
    head_(head),
    table_(table) {
  z1stats()->num_partitions_loaded.incr(1);
}

Partition::~Partition() {
  z1stats()->num_partitions_loaded.decr(1);
}

SHA1Hash Partition::uuid() const {
  auto snap = head_.getSnapshot();
  return snap->uuid();
}

RefPtr<PartitionWriter> Partition::getWriter() {
  std::unique_lock<std::mutex> lk(writer_lock_);
  if (writer_.get() == nullptr) {
    switch (table_->storage()) {

      case eventql::TBL_STORAGE_COLSM:
        if (upgradeToLSMv2()) {
          writer_ = mkRef<PartitionWriter>(
              new LSMPartitionWriter(cfg_, this, &head_));
        } else {
          writer_ = mkRef<PartitionWriter>(new LogPartitionWriter(this, &head_));
        }
        break;

      case eventql::TBL_STORAGE_STATIC:
        writer_ = mkRef<PartitionWriter>(new StaticPartitionWriter(&head_));
        break;

      default:
        RAISE(kRuntimeError, "invalid storage class");

    }
  }

  auto writer = writer_;
  return writer;
}

RefPtr<PartitionReader> Partition::getReader() {
  switch (table_->storage()) {

    case eventql::TBL_STORAGE_COLSM:
      if (upgradeToLSMv2()) {
        return new LSMPartitionReader(table_, head_.getSnapshot());
      } else {
        return new LogPartitionReader(table_, head_.getSnapshot());
      }

    case eventql::TBL_STORAGE_STATIC:
      return new StaticPartitionReader(table_, head_.getSnapshot());

    default:
      RAISE(kRuntimeError, "invalid storage class");

  }
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

RefPtr<PartitionReplication> Partition::getReplicationStrategy(
    RefPtr<ReplicationScheme> repl_scheme,
    http::HTTPConnectionPool* http) {
  switch (table_->storage()) {

    case eventql::TBL_STORAGE_COLSM:
      if (upgradeToLSMv2()) {
        return new LSMPartitionReplication(
            this,
            repl_scheme,
            cfg_->config_directory,
            http);
      } else {
        return new LogPartitionReplication(
            this,
            repl_scheme,
            http);
      }

    case eventql::TBL_STORAGE_STATIC:
      return new StaticPartitionReplication(
          this,
          repl_scheme,
          http);

    default:
      RAISE(kRuntimeError, "invalid storage class");

  }
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

}
