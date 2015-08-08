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

void TSDBService::createTable(const TableDefinition& table) {
  pmap_->configureTable(table);
}

void TSDBService::insertRecords(const RecordEnvelopeList& record_list) {
  Vector<RefPtr<Partition>> partition_refs;
  HashMap<Partition*, Vector<RecordRef>> grouped;
  for (const auto& record : record_list.records()) {
    SHA1Hash partition_key;

    if (record.has_partition_sha1()) {
      partition_key = SHA1Hash::fromHexString(record.partition_sha1());
    } else {
      auto table = pmap_->findTable(
          record.tsdb_namespace(),
          record.table_name());

      if (table.isEmpty()) {
        RAISEF(kNotFoundError, "table not found: $0", record.table_name());
      }

      partition_key = table.get()->partitioner()->partitionKeyFor(
          record.partition_key());
    }

    auto partition = pmap_->findOrCreatePartition(
        record.tsdb_namespace(),
        record.table_name(),
        partition_key);

    auto record_data = record.record_data().data();
    auto record_size = record.record_data().size();

    if (grouped.count(partition.get()) == 0) {
      partition_refs.emplace_back(partition);
    }

    grouped[partition.get()].emplace_back(
        SHA1Hash::fromHexString(record.record_id()),
        Buffer(record_data, record_size));
  }

  for (const auto& group : grouped) {
    auto partition = group.first;
    auto inserted = partition->getWriter()->insertRecords(group.second);

    if (inserted.size() > 0) {
      auto change = mkRef(new PartitionChangeNotification());
      change->partition = partition;
      pmap_->publishPartitionChange(change);
    }
  }
}

void TSDBService::insertRecord(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    const SHA1Hash& record_id,
    const Buffer& record) {
  auto partition = pmap_->findOrCreatePartition(
      tsdb_namespace,
      table_name,
      partition_key);

  auto writer = partition->getWriter();
  auto inserted = writer->insertRecord(record_id, record);

  if (inserted) {
    auto change = mkRef(new PartitionChangeNotification());
    change->partition = partition;
    pmap_->publishPartitionChange(change);
  }
}

void TSDBService::updatePartitionCSTable(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    const String& tmpfile_path,
    uint64_t version) {
  auto partition = pmap_->findOrCreatePartition(
      tsdb_namespace,
      table_name,
      partition_key);

  auto writer = partition->getWriter();
  writer->updateCSTable(tmpfile_path, version);

  auto change = mkRef(new PartitionChangeNotification());
  change->partition = partition;
  pmap_->publishPartitionChange(change);
}

void TSDBService::fetchPartition(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    Function<void (const Buffer& record)> fn) {
  fetchPartitionWithSampling(
      tsdb_namespace,
      table_name,
      partition_key,
      0,
      0,
      fn);
}

void TSDBService::fetchPartitionWithSampling(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    size_t sample_modulo,
    size_t sample_index,
    Function<void (const Buffer& record)> fn) {
  auto partition = pmap_->findPartition(
      tsdb_namespace,
      table_name,
      partition_key);

  if (partition.isEmpty()) {
    return;
  }

  auto reader = partition.get()->getReader();
  reader->fetchRecordsWithSampling(sample_modulo, sample_index, fn);
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

Option<RefPtr<msg::MessageSchema>> TSDBService::tableSchema(
    const String& tsdb_namespace,
    const String& table_key) {
  auto table = pmap_->findTable(
      tsdb_namespace,
      table_key);

  if (table.isEmpty()) {
    return None<RefPtr<msg::MessageSchema>>();
  } else {
    return Some(table.get()->schema());
  }
}

Option<TableDefinition> TSDBService::tableConfig(
    const String& tsdb_namespace,
    const String& table_key) {
  auto table = pmap_->findTable(
      tsdb_namespace,
      table_key);

  if (table.isEmpty()) {
    return None<TableDefinition>();
  } else {
    return Some(table.get()->config());
  }
}

//const String& TSDBService::dbPath() const {
//  return db_path_;
//}

} // namespace tdsb

