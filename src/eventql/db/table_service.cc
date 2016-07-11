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
#include <eventql/util/util/Base64.h>
#include <eventql/util/fnv.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/util/protobuf/MessageEncoder.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/wallclock.h>
#include <eventql/io/sstable/sstablereader.h>
#include <eventql/db/table_service.h>
#include <eventql/db/LogPartitionReader.h>
#include <eventql/db/PartitionState.pb.h>
#include "eventql/db/metadata_coordinator.h"
#include "eventql/db/metadata_file.h"
#include "eventql/db/metadata_client.h"
#include "eventql/db/server_allocator.h"

#include "eventql/eventql.h"

namespace eventql {

TableService::TableService(
    ConfigDirectory* cdir,
    PartitionMap* pmap,
    ReplicationScheme* repl,
    thread::EventLoop* ev,
    http::HTTPClientStats* http_stats) :
    cdir_(cdir),
    pmap_(pmap),
    repl_(repl),
    http_(ev, http_stats) {}

Status TableService::createTable(
    const String& db_namespace,
    const String& table_name,
    const msg::MessageSchema& schema,
    Vector<String> primary_key) {

  if (primary_key.size() < 1) {
    return Status(
        eIllegalArgumentError,
        "can't create table without PRIMARY KEY");
  }

  for (const auto& col : primary_key) {
    if (col.find(".") != String::npos) {
      return Status(
          eIllegalArgumentError,
          StringUtil::format(
              "nested column '$0' can't be part of the PRIMARY KEY",
              col));
    }

    if (!schema.hasField(col)) {
      return Status(
          eIllegalArgumentError,
          StringUtil::format(
              "column not found '$0'",
              col));
    }
  }

  String partition_key = primary_key[0];
  TablePartitionerType partitioner_type;
  switch (schema.fieldType(schema.fieldId(partition_key))) {
    case msg::FieldType::DATETIME:
      partitioner_type = TBL_PARTITION_TIMEWINDOW;
      break;
    case msg::FieldType::STRING:
      partitioner_type = TBL_PARTITION_STRING;
      break;
    case msg::FieldType::UINT64:
      partitioner_type = TBL_PARTITION_UINT64;
      break;
    default:
      return Status(
          eIllegalArgumentError,
          "first column in the PRIMARY KEY must be of type DATETIME, STRNG or UINT64");
  }

  auto replication_factor = cdir_->getClusterConfig().replication_factor();
  // generate new metadata file
  Set<String> servers;
  ServerAllocator server_alloc(cdir_);
  {
    auto rc = server_alloc.allocateServers(replication_factor, &servers);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  MetadataFile::PartitionMapEntry initial_partition;
  initial_partition.begin = "";
  initial_partition.partition_id = Random::singleton()->sha1();
  initial_partition.splitting = false;
  for (const auto& s : servers) {
    MetadataFile::PartitionPlacement p;
    p.server_id = s;
    p.placement_id = Random::singleton()->random64();
    initial_partition.servers.emplace_back(p);
  }

  auto txnid = Random::singleton()->sha1();
  MetadataFile metadata_file(txnid, 1, KEYSPACE_UINT64, { initial_partition });

  // generate new table config
  TableDefinition td;
  td.set_customer(db_namespace);
  td.set_table_name(table_name);
  td.set_metadata_txnid(txnid.data(), txnid.size());
  td.set_metadata_txnseq(1);
  for (const auto& s : servers) {
    td.add_metadata_servers(s);
  }

  auto tblcfg = td.mutable_config();
  tblcfg->set_schema(schema.encode().toString());
  tblcfg->set_num_shards(1);
  tblcfg->set_partitioner(partitioner_type);
  tblcfg->set_storage(eventql::TBL_STORAGE_COLSM);
  tblcfg->set_partition_key(partition_key);
  for (const auto& col : primary_key) {
    tblcfg->add_primary_key(col);
  }

  // create metadata file on metadata servers
  eventql::MetadataCoordinator coordinator(cdir_);
  auto rc = coordinator.createFile(
      db_namespace,
      table_name,
      metadata_file,
      Vector<String>(servers.begin(), servers.end()));

  if (!rc.isSuccess()) {
    return rc;
  }

  // create table config
  cdir_->updateTableConfig(td);
  return Status::success();
}


static Status addColumn(
    TableDefinition* td,
    TableService::AlterTableOperation operation) {
  auto schema = msg::MessageSchema::decode(td->config().schema());
  auto cur_schema = schema;
  auto field = operation.field_name;

  uint32_t next_field_id;
  if (td->has_next_field_id()) {
    next_field_id = td->next_field_id();
  } else {
    next_field_id = schema->maxFieldId() + 1;
  }

  while (StringUtil::includes(field, ".")) {
    auto prefix_len = field.find(".");
    auto prefix = field.substr(0, prefix_len);

    field = field.substr(prefix_len + 1);
    if (!cur_schema->hasField(prefix)) {
      return Status(
          eNotFoundError,
          StringUtil::format("field '$0' not found", field));
    }

    auto parent_field_id = cur_schema->fieldId(prefix);
    auto parent_field_type = cur_schema->fieldType(parent_field_id);
    if (parent_field_type != msg::FieldType::OBJECT) {
      return Status(
          eRuntimeError,
          StringUtil::format(
              "can't add a field to field '$0' of type $1",
              prefix,
              fieldTypeToString(parent_field_type)));
    }

    cur_schema = cur_schema->fieldSchema(parent_field_id);
  }

  if (cur_schema->hasField(field)) {
    return Status(
        eRuntimeError,
        StringUtil::format("column '$0' already exists ", operation.field_name));
  }

  if (operation.field_type == msg::FieldType::OBJECT) {
    cur_schema->addField(
          msg::MessageSchemaField::mkObjectField(
              next_field_id,
              field,
              operation.is_repeated,
              operation.is_optional,
              mkRef(new msg::MessageSchema(nullptr))));


  } else {
    cur_schema->addField(
          msg::MessageSchemaField(
              next_field_id,
              field,
              operation.field_type,
              0,
              operation.is_repeated,
              operation.is_optional));
  }


  td->set_next_field_id(next_field_id + 1);
  td->mutable_config()->set_schema(schema->encode().toString());
  return Status::success();
}

static Status removeColumn(
    TableDefinition* td,
    const Vector<String>& primary_key,
    const String& field_name) {
  if (find(primary_key.begin(), primary_key.end(), field_name) !=
      primary_key.end()) {
    return Status(eRuntimeError, "field with primary key can't be removed");
  }

  auto schema = msg::MessageSchema::decode(td->config().schema());
  auto cur_schema = schema;
  auto field = field_name;

  while (StringUtil::includes(field, ".")) {
    auto prefix_len = field.find(".");
    auto prefix = field.substr(0, prefix_len);

    field = field.substr(prefix_len + 1);

    if (!cur_schema->hasField(prefix)) {
      return Status(
          eNotFoundError,
          StringUtil::format("field '$0' not found", prefix));
    }
    cur_schema = cur_schema->fieldSchema(cur_schema->fieldId(prefix));
  }

  if (!cur_schema->hasField(field)) {
    return Status(
        eNotFoundError,
        StringUtil::format("field '$0' not found", field));
  }

  if (!td->has_next_field_id()) {
    td->set_next_field_id(schema->maxFieldId() + 1);
  }

  cur_schema->removeField(cur_schema->fieldId(field));
  td->mutable_config()->set_schema(schema->encode().toString());
  return Status::success();
}

Status TableService::alterTable(
    const String& db_namespace,
    const String& table_name,
    Vector<TableService::AlterTableOperation> operations) {
  auto table = pmap_->findTable(db_namespace, table_name);
  if (table.isEmpty()) {
    return Status(eNotFoundError, "table not found");
  }

  auto primary_key = table.get()->getPrimaryKey();
  auto td = table.get()->config();

  for (auto o : operations) {
    if (o.optype == AlterTableOperationType::OP_ADD_COLUMN) {
      auto rc = addColumn(&td, o);
      if (!rc.isSuccess()) {
        return rc;
      }
    } else {
      auto rc = removeColumn(&td, primary_key, o.field_name);
      if (!rc.isSuccess()) {
        return rc;
      }
    }
  }

  try {
    cdir_->updateTableConfig(td);
  } catch (const Exception& e) {
    return Status(eRuntimeError, e.getMessage());
  }

  return Status::success();
}

void TableService::listTables(
    const String& tsdb_namespace,
    Function<void (const TSDBTableInfo& table)> fn) const {
  pmap_->listTables(
      tsdb_namespace,
      [this, fn] (const TSDBTableInfo& table) {
    fn(table);
  });
}

void TableService::listTablesReverse(
    const String& tsdb_namespace,
    Function<void (const TSDBTableInfo& table)> fn) const {
  pmap_->listTablesReverse(
      tsdb_namespace,
      [this, fn] (const TSDBTableInfo& table) {
    fn(table);
  });
}

void TableService::insertRecord(
    const String& tsdb_namespace,
    const String& table_name,
    const json::JSONObject::const_iterator& data_begin,
    const json::JSONObject::const_iterator& data_end,
    uint64_t flags /* = 0 */) {
  auto table = pmap_->findTable(tsdb_namespace, table_name);
  if (table.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", table_name);
  }

  msg::DynamicMessage record(table.get()->schema());
  record.fromJSON(data_begin, data_end);
  insertRecord(
      tsdb_namespace,
      table_name,
      record,
      flags);
}

void TableService::insertRecord(
    const String& tsdb_namespace,
    const String& table_name,
    const msg::DynamicMessage& data,
    uint64_t flags /* = 0 */) {
  insertRecords(
      tsdb_namespace,
      table_name,
      &data,
      &data + 1,
      flags);
}

void TableService::insertRecords(
    const String& tsdb_namespace,
    const String& table_name,
    const msg::DynamicMessage* begin,
    const msg::DynamicMessage* end,
    uint64_t flags /* = 0 */) {
  MetadataClient metadata_client(cdir_);
  HashMap<SHA1Hash, Vector<RecordRef>> records;
  HashMap<SHA1Hash, Set<String>> servers;

  auto table = pmap_->findTable(tsdb_namespace, table_name);
  if (table.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", table_name);
  }

  for (auto record = begin; record != end; ++record) {
    // calculate partition key
    auto partition_key_field_name = table.get()->getPartitionKey();
    auto partition_key_field = record->getField(partition_key_field_name);
    if (partition_key_field.isEmpty()) {
      RAISEF(kNotFoundError, "missing field: $0", partition_key_field_name);
    }

    // calculate primary key
    SHA1Hash primary_key;
    auto primary_key_columns = table.get()->getPrimaryKey();
    switch (primary_key_columns.size()) {

      // no primary key definition. key value is random SHA1
      case 0: {
        primary_key = Random::singleton()->sha1();
        break;
      }

      // simple primary key, key value is SHA1 of column value
      case 1: {
        auto f = record->getField(primary_key_columns[0]);
        if (f.isEmpty()) {
          RAISEF(kNotFoundError, "missing field: $0", primary_key_columns[0]);
        }
        primary_key = SHA1::compute(f.get());
        break;
      }

      // compund primary key, key value is chained SHA1 of column values
      default: {
        for (const auto& c : primary_key_columns) {
          auto f = record->getField(c);
          if (f.isEmpty()) {
            RAISEF(kNotFoundError, "missing field: $0", c);
          }

          auto chash = SHA1::compute(f.get());
          Buffer primary_key_data;
          primary_key_data.append(primary_key.data(), primary_key.size());
          primary_key_data.append(chash.data(), chash.size());
          primary_key = SHA1::compute(primary_key_data);
        }
        break;
      }

    }

    // lookup partition
    PartitionFindResponse find_res;
    {
      auto rc = metadata_client.findPartition(
          tsdb_namespace,
          table_name,
          encodePartitionKey(
              table.get()->getKeyspaceType(),
              partition_key_field.get()),
          &find_res);

      if (!rc.isSuccess()) {
        RAISE(kRuntimeError, rc.message());
      }
    }

    SHA1Hash partition_id(
        find_res.partition_id().data(),
        find_res.partition_id().size());

    Set<String> partition_servers(
        find_res.servers_for_insert().begin(),
        find_res.servers_for_insert().end());

    // FIXME
    Buffer buf;
    msg::MessageEncoder::encode(record->data(), *record->schema(), &buf);

    servers[partition_id] = partition_servers;
    records[partition_id].emplace_back(
        primary_key,
        WallClock::unixMicros(),
        buf);
  }

  for (const auto& p : records) {
    insertRecords(
        tsdb_namespace,
        table_name,
        p.first,
        servers[p.first],
        p.second,
        flags);
  }
}

void TableService::insertReplicatedRecords(
    const RecordEnvelopeList& records,
    uint64_t flags /* = 0 */) {
  HashMap<String, Vector<RecordRef>> grouped;

  for (const auto& record : records.records()) {
    if (!record.has_partition_sha1()) {
      RAISE(kIllegalArgumentError, "missing partition id");
    }

    auto partition_key = SHA1Hash::fromHexString(record.partition_sha1());
    auto record_data = record.record_data().data();
    auto record_size = record.record_data().size();
    auto record_version = record.record_version();

    auto group_key = StringUtil::format(
        "$0~$1~$2",
        record.tsdb_namespace(),
        record.table_name(),
        partition_key.toString());

    grouped[group_key].emplace_back(
        SHA1Hash::fromHexString(record.record_id()),
        record_version,
        Buffer(record_data, record_size));
  }

  for (const auto& group : grouped) {
    auto group_key = StringUtil::split(group.first, "~");
    if (group_key.size() != 3) {
      RAISE(kIllegalStateError);
    }

    insertRecordsLocal(
        group_key[0],
        group_key[1],
        SHA1Hash::fromHexString(group_key[2]),
        group.second,
        flags);
  }
}

void TableService::insertRecords(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    const Set<String>& servers,
    const Vector<RecordRef>& records,
    uint64_t flags /* = 0 */) {
  Vector<String> errors;

  size_t nconfirmations = 0;
  for (const auto& server : servers) {
    try {
      if (server == cdir_->getServerID()) {
        insertRecordsLocal(
            tsdb_namespace,
            table_name,
            partition_key,
            records,
            flags);
      } else {
        insertRecordsRemote(
            tsdb_namespace,
            table_name,
            partition_key,
            records,
            flags,
            server);
      }

      ++nconfirmations;
    } catch (const StandardException& e) {
      logError(
          "eventql",
          e,
          "TableService::insertRecordsRemote failed");

      errors.emplace_back(e.what());
    }
  }

  if (nconfirmations < 1) { // FIXME min consistency level
    RAISEF(
        kRuntimeError,
        "TableService::insertRecordsRemote failed: $0",
        StringUtil::join(errors, ", "));
  }
}

void TableService::insertRecordsLocal(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    const Vector<RecordRef>& records,
    uint64_t flags) {
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
    if (writer->insertRecord(r.record_id, r.record_version, r.record)) {
      dirty = true;
    }
  }

  if (dirty) {
    if (flags & (uint64_t) InsertFlags::SYNC_COMMIT) {
      writer->commit();
    }

    auto change = mkRef(new PartitionChangeNotification());
    change->partition = partition;
    pmap_->publishPartitionChange(change);
  }
}

void TableService::insertRecordsRemote(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    const Vector<RecordRef>& records,
    uint64_t flags,
    const String& server_id) {
  auto server_cfg = cdir_->getServerConfig(server_id);
  if (server_cfg.server_status() != SERVER_UP) {
    RAISE(kRuntimeError, "server is down");
  }

  logDebug(
      "z1.core",
      "Inserting $0 records into $1:$2/$3/$4",
      records.size(),
      server_id,
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
    record->set_record_version(r.record_version);
  }

  auto uri = URI(StringUtil::format(
      "http://$0/tsdb/replicate",
      server_cfg.server_addr()));

  if (flags & (uint64_t) InsertFlags::SYNC_COMMIT) {
    envelope.set_sync_commit(true);
  }

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

void TableService::compactPartition(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key) {
  auto partition = pmap_->findOrCreatePartition(
      tsdb_namespace,
      table_name,
      partition_key);

  auto writer = partition->getWriter();
  if (writer->compact()) {
    auto change = mkRef(new PartitionChangeNotification());
    change->partition = partition;
    pmap_->publishPartitionChange(change);
  }
}

void TableService::commitPartition(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key) {
  auto partition = pmap_->findOrCreatePartition(
      tsdb_namespace,
      table_name,
      partition_key);

  auto writer = partition->getWriter();
  if (writer->commit()) {
    auto change = mkRef(new PartitionChangeNotification());
    change->partition = partition;
    pmap_->publishPartitionChange(change);
  }
}

void TableService::updatePartitionCSTable(
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

void TableService::fetchPartition(
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

void TableService::fetchPartitionWithSampling(
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
  auto log_reader = dynamic_cast<LogPartitionReader*>(reader.get());
  if (!log_reader) {
    RAISE(
        kRuntimeError,
        "raw record fetches are supported only on LOG partitions");
  }

  log_reader->fetchRecordsWithSampling(sample_modulo, sample_index, fn);
}

Option<RefPtr<msg::MessageSchema>> TableService::tableSchema(
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

Option<TableDefinition> TableService::tableConfig(
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

} // namespace tdsb

