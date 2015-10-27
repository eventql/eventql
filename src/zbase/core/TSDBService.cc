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
#include <stx/protobuf/MessageEncoder.h>
#include <stx/io/fileutil.h>
#include <sstable/sstablereader.h>
#include <zbase/core/TSDBService.h>
#include <zbase/core/TimeWindowPartitioner.h>
#include <zbase/core/PartitionState.pb.h>

using namespace stx;

namespace zbase {

TSDBService::TSDBService(
    PartitionMap* pmap,
    ReplicationScheme* repl,
    thread::EventLoop* ev) :
    pmap_(pmap),
    repl_(repl),
    http_(ev) {}

void TSDBService::createTable(const TableDefinition& table) {
  pmap_->configureTable(table);
}

void TSDBService::insertRecords(const RecordEnvelopeList& record_list) {
  Vector<RefPtr<Partition>> partition_refs;
  HashMap<String, Vector<RecordRef>> grouped;

  for (const auto& record : record_list.records()) {
    SHA1Hash partition_key;

    auto table = pmap_->findTable(
        record.tsdb_namespace(),
        record.table_name());

    if (table.isEmpty()) {
      RAISEF(kNotFoundError, "table not found: $0", record.table_name());
    }

    if (record.has_partition_sha1()) {
      partition_key = SHA1Hash::fromHexString(record.partition_sha1());
    } else {
      partition_key = table.get()->partitioner()->partitionKeyFor(
          record.partition_key());
    }

    auto record_data = record.record_data().data();
    auto record_size = record.record_data().size();

    auto group_key = StringUtil::format(
        "$0~$1~$2",
        record.tsdb_namespace(),
        record.table_name(),
        partition_key.toString());

    grouped[group_key].emplace_back(
        SHA1Hash::fromHexString(record.record_id()),
        Buffer(record_data, record_size));
  }

  for (const auto& group : grouped) {
    auto group_key = StringUtil::split(group.first, "~");
    if (group_key.size() != 3) {
      RAISE(kIllegalStateError);
    }

    insertRecords(
        group_key[0],
        group_key[1],
        SHA1Hash::fromHexString(group_key[2]),
        group.second);
  }
}

void TSDBService::insertRecords(const Vector<RecordEnvelope>& records) {
  Vector<RefPtr<Partition>> partition_refs;
  HashMap<String, Vector<RecordRef>> grouped;

  for (const auto& record : records) {
    SHA1Hash partition_key;

    auto table = pmap_->findTable(
        record.tsdb_namespace(),
        record.table_name());

    if (table.isEmpty()) {
      RAISEF(kNotFoundError, "table not found: $0", record.table_name());
    }

    if (record.has_partition_sha1()) {
      partition_key = SHA1Hash::fromHexString(record.partition_sha1());
    } else {
      partition_key = table.get()->partitioner()->partitionKeyFor(
          record.partition_key());
    }

    auto record_data = record.record_data().data();
    auto record_size = record.record_data().size();

    auto group_key = StringUtil::format(
        "$0~$1~$2",
        record.tsdb_namespace(),
        record.table_name(),
        partition_key.toString());

    grouped[group_key].emplace_back(
        SHA1Hash::fromHexString(record.record_id()),
        Buffer(record_data, record_size));
  }

  for (const auto& group : grouped) {
    auto group_key = StringUtil::split(group.first, "~");
    if (group_key.size() != 3) {
      RAISE(kIllegalStateError);
    }

    insertRecords(
        group_key[0],
        group_key[1],
        SHA1Hash::fromHexString(group_key[2]),
        group.second);
  }
}

void TSDBService::insertRecord(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    const SHA1Hash& record_id,
    const Buffer& record) {
  Vector<RecordRef> records;
  records.emplace_back(record_id, record);
  insertRecords(tsdb_namespace, table_name, partition_key, records);
}

void TSDBService::insertRecord(
    const String& tsdb_namespace,
    const String& table_name,
    const json::JSONObject::const_iterator& data_begin,
    const json::JSONObject::const_iterator& data_end) {
  auto table = pmap_->findTable(tsdb_namespace, table_name);
  if (table.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", table_name);
  }

  msg::DynamicMessage record(table.get()->schema());
  record.fromJSON(data_begin, data_end);
  insertRecord(tsdb_namespace, table_name, record);
}

void TSDBService::insertRecord(
    const String& tsdb_namespace,
    const String& table_name,
    const msg::DynamicMessage& data) {
  auto table = pmap_->findTable(tsdb_namespace, table_name);
  if (table.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", table_name);
  }

  auto partition_key_field_name = "time";
  auto partition_key_field = data.getField(partition_key_field_name);
  if (partition_key_field.isEmpty()) {
    RAISEF(kNotFoundError, "missing field: $0", partition_key_field_name);
  }

  auto partitioner = table.get()->partitioner();
  auto partition_key = partitioner->partitionKeyFor(partition_key_field.get());

  auto record_id = Random::singleton()->sha1();

  Buffer record;
  msg::MessageEncoder::encode(data.data(), *data.schema(), &record);

  insertRecord(
      tsdb_namespace,
      table_name,
      partition_key,
      record_id,
      record);
}

void TSDBService::insertRecords(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    const Vector<RecordRef>& records) {
  Vector<String> errors;
  auto hosts = repl_->replicasFor(partition_key);
  for (const auto& host : hosts) {

    try {
      if (host.is_local) {
        insertRecordsLocal(
            tsdb_namespace,
            table_name,
            partition_key,
            records);
      } else {
        insertRecordsRemote(
            tsdb_namespace,
            table_name,
            partition_key,
            records,
            host);
      }

      return;
    } catch (const StandardException& e) {
      logError(
          "zbase",
          e,
          "TSDBService::insertRecordsRemote failed");

      errors.emplace_back(e.what());
    }
  }

  RAISEF(
      kRuntimeError,
      "TSDBService::insertRecordsRemote failed: $0",
      StringUtil::join(errors, ", "));
}

void TSDBService::insertRecordsLocal(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    const Vector<RecordRef>& records) {
  logDebug(
      "z1.core",
      "Inserting $0 records into tsdb://localhost/$1/$2/$3",
      records.size(),
      tsdb_namespace,
      table_name,
      partition_key.toString());

  auto partition = pmap_->findOrCreatePartition(
      tsdb_namespace,
      table_name,
      partition_key);

  auto writer = partition->getWriter();

  bool dirty = false;
  for (const auto& r : records) {
    if (writer->insertRecord(r.record_id, r.record)) {
      dirty = true;
    }
  }

  if (dirty) {
    auto change = mkRef(new PartitionChangeNotification());
    change->partition = partition;
    pmap_->publishPartitionChange(change);
  }
}

void TSDBService::insertRecordsRemote(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    const Vector<RecordRef>& records,
    const ReplicaRef& host) {
  logDebug(
      "z1.core",
      "Inserting $0 records into tsdb://$1/$2/$3/$4",
      records.size(),
      host.name,
      tsdb_namespace,
      table_name,
      partition_key.toString());

  RecordEnvelopeList envelope;
  for (const auto& r : records) {
    auto record = envelope.add_records();
    record->set_tsdb_namespace(tsdb_namespace);
    record->set_table_name(table_name);
    record->set_partition_sha1(partition_key.toString());
    record->set_record_id(r.record_id.toString());
    record->set_record_data(r.record.toString());
  }

  auto uri = URI(StringUtil::format(
      "http://$0/tsdb/insert",
      host.addr.ipAndPort()));

  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addBody(*msg::encode(envelope));

  auto res = http_.executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 201) {
    RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
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

Option<RefPtr<TablePartitioner>> TSDBService::tablePartitioner(
    const String& tsdb_namespace,
    const String& table_key) {
  auto table = pmap_->findTable(
      tsdb_namespace,
      table_key);

  if (table.isEmpty()) {
    return None<RefPtr<TablePartitioner>>();
  } else {
    return Some(table.get()->partitioner());
  }
}

//const String& TSDBService::dbPath() const {
//  return db_path_;
//}

} // namespace tdsb

