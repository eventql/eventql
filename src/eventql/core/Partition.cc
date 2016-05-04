/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/core/Partition.h>
#include <stx/io/fileutil.h>
#include <stx/uri.h>
#include <stx/logging.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/wallclock.h>
#include <stx/protobuf/MessageEncoder.h>
#include <stx/protobuf/MessageDecoder.h>
#include <stx/protobuf/msg.h>
#include <eventql/infra/sstable/sstablereader.h>
#include <eventql/core/LogPartitionReader.h>
#include <eventql/core/LogPartitionWriter.h>
#include <eventql/core/LogPartitionReplication.h>
#include <eventql/core/LSMPartitionReader.h>
#include <eventql/core/LSMPartitionWriter.h>
#include <eventql/core/LSMPartitionReplication.h>
#include <eventql/core/StaticPartitionReader.h>
#include <eventql/core/StaticPartitionWriter.h>
#include <eventql/core/StaticPartitionReplication.h>
#include <eventql/z1stats.h>

using namespace stx;

namespace zbase {

RefPtr<Partition> Partition::create(
    const String& tsdb_namespace,
    RefPtr<Table> table,
    const SHA1Hash& partition_key,
    ServerConfig* cfg) {
  stx::logDebug(
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

  auto snap = mkRef(new PartitionSnapshot(state, pdir, pdir_rel, 0));
  snap->writeToDisk();
  return new Partition(cfg, snap, table);
}

RefPtr<Partition> Partition::reopen(
    const String& tsdb_namespace,
    RefPtr<Table> table,
    const SHA1Hash& partition_key,
    ServerConfig* cfg) {
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

  stx::logTrace(
      "tsdb",
      "Loading partition $0/$1/$2 ($3 records)",
      tsdb_namespace,
      table->name(),
      partition_key.toString(),
      nrecs);

  auto snap = mkRef(new PartitionSnapshot(state, pdir, pdir_rel, nrecs));
  return new Partition(cfg, snap, table);
}

Partition::Partition(
    ServerConfig* cfg,
    RefPtr<PartitionSnapshot> head,
    RefPtr<Table> table) :
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

      case zbase::TBL_STORAGE_COLSM:
        if (upgradeToLSMv2()) {
          writer_ = mkRef<PartitionWriter>(
              new LSMPartitionWriter(cfg_, this, &head_));
        } else {
          writer_ = mkRef<PartitionWriter>(new LogPartitionWriter(this, &head_));
        }
        break;

      case zbase::TBL_STORAGE_STATIC:
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

    case zbase::TBL_STORAGE_COLSM:
      if (upgradeToLSMv2()) {
        return new LSMPartitionReader(table_, head_.getSnapshot());
      } else {
        return new LogPartitionReader(table_, head_.getSnapshot());
      }

    case zbase::TBL_STORAGE_STATIC:
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

    case zbase::TBL_STORAGE_COLSM:
      if (upgradeToLSMv2()) {
        return new LSMPartitionReplication(
            this,
            repl_scheme,
            http);
      } else {
        return new LogPartitionReplication(
            this,
            repl_scheme,
            http);
      }

    case zbase::TBL_STORAGE_STATIC:
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

}
