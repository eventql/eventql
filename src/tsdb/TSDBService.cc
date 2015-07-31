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
#include <tsdb/TSDBService.h>
#include <tsdb/PartitionState.pb.h>

using namespace stx;

namespace tsdb {

TSDBService::TSDBService(PartitionMap* pmap) : pmap_(pmap) {}

Option<RefPtr<Table>> TSDBService::findTable(
    const String& stream_ns,
    const String& stream_key) const {
  return pmap_->findTable(stream_ns, stream_key);
}

void TSDBService::createTable(const TableConfig& table) {
  pmap_->configureTable(table);
}

void TSDBService::fetchPartition(
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

void TSDBService::fetchPartitionWithSampling(
    const String& tsdb_namespace,
    const String& stream_key,
    const SHA1Hash& partition_key,
    size_t sample_modulo,
    size_t sample_index,
    Function<void (const Buffer& record)> fn) {
  auto partition = pmap_->findPartition(
      tsdb_namespace,
      stream_key,
      partition_key);

  if (partition.isEmpty()) {
    return;
  }

  auto reader = partition.get()->getReader();
  reader->fetchRecordsWithSampling(sample_modulo, sample_index, fn);
}

void TSDBService::listTables(
    const String& tsdb_namespace,
    Function<void (const TSDBTableInfo& table)> fn) const {
  pmap_->listTables(tsdb_namespace, fn);
}

Option<TSDBTableInfo> TSDBService::tableInfo(
      const String& tsdb_namespace,
      const String& table_key) const {
  auto table = pmap_->findTable(tsdb_namespace, table_key);
  if (table.isEmpty()) {
    return None<TSDBTableInfo>();
  }

  TSDBTableInfo ti;
  ti.table_name = table.get()->name();
  ti.config = table.get()->config();
  ti.schema = table.get()->schema();
  return Some(ti);
}

Option<PartitionInfo> TSDBService::partitionInfo(
    const String& tsdb_namespace,
    const String& table_key,
    const SHA1Hash& partition_key) {
  auto partition = pmap_->findPartition(
      tsdb_namespace,
      table_key,
      partition_key);

  if (partition.isEmpty()) {
    return None<PartitionInfo>();
  } else {
    return Some(partition.get()->getInfo());
  }
}

//const String& TSDBService::dbPath() const {
//  return db_path_;
//}

} // namespace tdsb

