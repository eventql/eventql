/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <zbase/core/Partition.h>
#include <stx/io/fileutil.h>
#include <stx/uri.h>
#include <stx/logging.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/wallclock.h>
#include <stx/protobuf/MessageEncoder.h>
#include <stx/protobuf/MessageDecoder.h>
#include <stx/protobuf/msg.h>
#include <sstable/sstablereader.h>
#include <zbase/core/LogPartitionReader.h>
#include <zbase/core/LogPartitionWriter.h>
#include <zbase/core/LogPartitionReplication.h>
#include <zbase/core/LSMPartitionReader.h>
#include <zbase/core/LSMPartitionWriter.h>
#include <zbase/core/LSMPartitionReplication.h>
#include <zbase/core/StaticPartitionReader.h>
#include <zbase/core/StaticPartitionWriter.h>
#include <zbase/core/StaticPartitionReplication.h>
#include <zbase/z1stats.h>

using namespace stx;

namespace zbase {

RefPtr<Partition> Partition::create(
    const String& tsdb_namespace,
    RefPtr<Table> table,
    const SHA1Hash& partition_key,
    const String& db_path) {
  stx::logDebug(
      "tsdb",
      "Creating new partition; stream='$0' partition='$1'",
      table->name(),
      partition_key.toString());

  auto pdir = FileUtil::joinPaths(
      db_path,
      StringUtil::format(
          "$0/$1/$2",
          tsdb_namespace,
          SHA1::compute(table->name()).toString(),
          partition_key.toString()));

  FileUtil::mkdir_p(pdir);

  auto uuid = Random::singleton()->sha1();

  PartitionState state;
  state.set_tsdb_namespace(tsdb_namespace);
  state.set_partition_key(partition_key.data(), partition_key.size());
  state.set_table_key(table->name());
  state.set_uuid(uuid.data(), uuid.size());

  auto snap = mkRef(new PartitionSnapshot(state, pdir, 0));
  snap->writeToDisk();
  return new Partition(snap, table);
}

RefPtr<Partition> Partition::reopen(
    const String& tsdb_namespace,
    RefPtr<Table> table,
    const SHA1Hash& partition_key,
    const String& db_path) {
  auto pdir = FileUtil::joinPaths(
      db_path,
      StringUtil::format(
          "$0/$1/$2",
          tsdb_namespace,
          SHA1::compute(table->name()).toString(),
          partition_key.toString()));

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

  auto snap = mkRef(new PartitionSnapshot(state, pdir, nrecs));
  return new Partition(snap, table);
}

Partition::Partition(
    RefPtr<PartitionSnapshot> head,
    RefPtr<Table> table) :
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

      case zbase::TBL_STORAGE_LOG:
        writer_ = mkRef<PartitionWriter>(new LogPartitionWriter(&head_));
        break;

      case zbase::TBL_STORAGE_LSM:
        writer_ = mkRef<PartitionWriter>(new LSMPartitionWriter(&head_));
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

    case zbase::TBL_STORAGE_LOG:
      return new LogPartitionReader(table_, head_.getSnapshot());

    case zbase::TBL_STORAGE_LSM:
      return new LSMPartitionReader(table_, head_.getSnapshot());

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

    case zbase::TBL_STORAGE_LOG:
      return new LogPartitionReplication(
          this,
          repl_scheme,
          http);

    case zbase::TBL_STORAGE_LSM:
      return new LSMPartitionReplication(
          this,
          repl_scheme,
          http);

    case zbase::TBL_STORAGE_STATIC:
      return new StaticPartitionReplication(
          this,
          repl_scheme,
          http);

    default:
      RAISE(kRuntimeError, "invalid storage class");

  }
}

}
