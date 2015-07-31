/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/util/Base64.h>
#include <stx/fnv.h>
#include <stx/protobuf/msg.h>
#include <stx/io/fileutil.h>
#include <sstable/sstablereader.h>
#include <tsdb/TSDBNode.h>
#include <tsdb/PartitionState.pb.h>

using namespace stx;

namespace tsdb {

TSDBNode::TSDBNode(const String& db_path) :
    pmap_(db_path) {}

Option<RefPtr<Table>> TSDBNode::findTable(
    const String& stream_ns,
    const String& stream_key) const {
  return pmap_.findTable(stream_ns, stream_key);
}

void TSDBNode::createTable(const TableConfig& table) {
  pmap_.configureTable(table);
}

void TSDBNode::start(
    size_t num_compaction_threads,
    size_t num_replication_threads) {
  pmap_.open();

  //for (int i = 0; i < num_compaction_threads; ++i) {
  //  compaction_workers_.emplace_back(new CompactionWorker(&noderef_));
  //  compaction_workers_.back()->start();
  //}

  //for (int i = 0; i < num_replication_threads; ++i) {
  //  replication_workers_.emplace_back(new ReplicationWorker(&noderef_));
  //  replication_workers_.back()->start();
  //}
}

void TSDBNode::stop() {
  //for (auto& w : compaction_workers_) {
  //  w->stop();
  //}

  //for (auto& w : replication_workers_) {
  //  w->stop();
  //}
}

RefPtr<Partition> TSDBNode::findOrCreatePartition(
    const String& tsdb_namespace,
    const String& stream_key,
    const SHA1Hash& partition_key) {
  return pmap_.findOrCreatePartition(tsdb_namespace, stream_key, partition_key);
}

Option<RefPtr<Partition>> TSDBNode::findPartition(
    const String& tsdb_namespace,
    const String& stream_key,
    const SHA1Hash& partition_key) {
  return pmap_.findPartition(tsdb_namespace, stream_key, partition_key);
}

void TSDBNode::fetchPartition(
    const String& tsdb_namespace,
    const String& stream_key,
    const SHA1Hash& partition_key,
    Function<void (const Buffer& record)> fn) {
  fetchPartitionWithSampling(
      tsdb_namespace,
      stream_key,
      partition_key,
      0,
      0,
      fn);
}

void TSDBNode::fetchPartitionWithSampling(
    const String& tsdb_namespace,
    const String& stream_key,
    const SHA1Hash& partition_key,
    size_t sample_modulo,
    size_t sample_index,
    Function<void (const Buffer& record)> fn) {
  auto partition = pmap_.findPartition(
      tsdb_namespace,
      stream_key,
      partition_key);

  if (partition.isEmpty()) {
    return;
  }

  auto reader = partition.get()->getReader();
  reader->fetchRecordsWithSampling(sample_modulo, sample_index, fn);
}

void TSDBNode::listTables(
    const String& tsdb_namespace,
    Function<void (const TSDBTableInfo& table)> fn) const {
  pmap_.listTables(tsdb_namespace, fn);
}

Option<TSDBTableInfo> TSDBNode::tableInfo(
      const String& tsdb_namespace,
      const String& table_key) const {
  auto table = pmap_.findTable(tsdb_namespace, table_key);
  if (table.isEmpty()) {
    return None<TSDBTableInfo>();
  }

  TSDBTableInfo ti;
  ti.table_name = table.get()->name();
  ti.config = table.get()->config();
  ti.schema = table.get()->schema();
  return Some(ti);
}

Option<PartitionInfo> TSDBNode::partitionInfo(
    const String& tsdb_namespace,
    const String& table_key,
    const SHA1Hash& partition_key) {
  auto partition = pmap_.findPartition(
      tsdb_namespace,
      table_key,
      partition_key);

  if (partition.isEmpty()) {
    return None<PartitionInfo>();
  } else {
    return Some(partition.get()->getInfo());
  }
}

//const String& TSDBNode::dbPath() const {
//  return db_path_;
//}

} // namespace tdsb

