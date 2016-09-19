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
#include <eventql/util/util/Base64.h>
#include <algorithm>
#include <eventql/util/fnv.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/util/protobuf/MessageEncoder.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/wallclock.h>
#include <eventql/io/sstable/sstablereader.h>
#include <eventql/db/table_service.h>
#include <eventql/db/partition_state.pb.h>
#include <eventql/db/partition_reader.h>
#include <eventql/db/partition_writer.h>
#include "eventql/db/metadata_coordinator.h"
#include "eventql/db/metadata_file.h"
#include "eventql/db/metadata_client.h"
#include "eventql/db/server_allocator.h"
#include <eventql/transport/native/client_tcp.h>
#include <eventql/transport/native/frames/repl_insert.h>

#include "eventql/eventql.h"

namespace eventql {

TableService::TableService(
    ConfigDirectory* cdir,
    PartitionMap* pmap,
    ProcessConfig* config) :
    cdir_(cdir),
    pmap_(pmap),
    config_(config) {}

Status TableService::createTable(
    const String& db_namespace,
    const String& table_name,
    const msg::MessageSchema& schema,
    Vector<String> primary_key,
    const std::vector<std::pair<std::string, std::string>>& properties) {
  if (primary_key.size() < 1) {
    return Status(
        eIllegalArgumentError,
        "can't create table without PRIMARY KEY");
  }

  uint64_t recreate_version = 0;
  auto old_table = pmap_->findTable(db_namespace, table_name);
  if (!old_table.isEmpty()) {
    if (old_table.get()->config().deleted()) {
      recreate_version = old_table.get()->config().version();
    } else {
      return Status(eIllegalArgumentError, "table already exists");
    }
  }

  auto fields = schema.fields();
  for (const auto& col : primary_key) {
    if (col.find(".") != String::npos) {
      return Status(
          eIllegalArgumentError,
          StringUtil::format(
              "nested column '$0' can't be part of the PRIMARY KEY",
              col));
    }

    uint32_t field_id;
    try {
      field_id = schema.fieldId(col);
    } catch (const Exception& e) {
      return Status(
          eIllegalArgumentError,
          StringUtil::format("column not found: '$0'", col));
    }

    for (const auto& field : fields) {
      if (field_id == field.id) {
        if (field.type == msg::FieldType::OBJECT) {
          return Status(
              eIllegalArgumentError,
              StringUtil::format(
                  "nested column '$0' can't be part of the PRIMARY KEY",
                  col));
        }

        if (field.repeated) {
          return Status(
              eIllegalArgumentError,
              StringUtil::format(
                  "repeated column '$0' can't be part of the PRIMARY KEY",
                  col));
        }
      }
    }
  }

  String partition_key = primary_key[0];
  TablePartitionerType partitioner_type;
  KeyspaceType keyspace_type;
  switch (schema.fieldType(schema.fieldId(partition_key))) {
    case msg::FieldType::DATETIME:
      partitioner_type = TBL_PARTITION_TIMEWINDOW;
      keyspace_type = KEYSPACE_UINT64;
      break;
    case msg::FieldType::STRING:
      partitioner_type = TBL_PARTITION_STRING;
      keyspace_type = KEYSPACE_STRING;
      break;
    case msg::FieldType::UINT64:
      partitioner_type = TBL_PARTITION_UINT64;
      keyspace_type = KEYSPACE_UINT64;
      break;
    default:
      return Status(
          eIllegalArgumentError,
          "first column in the PRIMARY KEY must be of type DATETIME, STRING or UINT64");
  }

  // generate new table config
  TableDefinition td;
  td.set_version(recreate_version);
  td.set_customer(db_namespace);
  td.set_table_name(table_name);

  auto tblcfg = td.mutable_config();
  tblcfg->set_schema(schema.encode().toString());
  tblcfg->set_num_shards(1);
  tblcfg->set_partitioner(partitioner_type);
  tblcfg->set_storage(eventql::TBL_STORAGE_COLSM);
  tblcfg->set_partition_key(partition_key);

  for (const auto& col : primary_key) {
    tblcfg->add_primary_key(col);
  }

  for (const auto& p : properties) {
    if (p.first == "partition_size_hint") {
      uint64_t val = 0;
      try {
        val = std::stoull(p.second);
      } catch (...) {}

      tblcfg->set_enable_finite_partitions(true);
      tblcfg->set_finite_partition_size(val);
      continue;
    }
  }

  // check preconditions
  if (tblcfg->enable_finite_partitions()) {
    if (tblcfg->finite_partition_size() < 1) {
        return Status(
            eIllegalArgumentError,
            "finite partition size must be > 0");
    }

    switch (keyspace_type) {
      case KEYSPACE_UINT64:
        break;
      case KEYSPACE_STRING:
        return Status(
            eIllegalArgumentError,
            "can't set finite partition size for string partition keys");
    }
  }

  auto cconf = cdir_->getClusterConfig();

  // generate new metadata file
  std::vector<String> servers;
  ServerAllocator server_alloc(cdir_);
  {
    auto rc = server_alloc.allocateServers(
        ServerAllocator::MUST_ALLOCATE,
        cconf.replication_factor(),
        Set<String>{},
        &servers);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  auto txnid = Random::singleton()->sha1();
  std::unique_ptr<MetadataFile> metadata_file;
  if (tblcfg->enable_finite_partitions()) {
    metadata_file.reset(
        new MetadataFile(
            txnid,
            1,
            keyspace_type,
            { },
            MFILE_FINITE));
  } else {
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

    metadata_file.reset(
        new MetadataFile(txnid, 1, keyspace_type, { initial_partition }, 0));
  }

  td.set_metadata_txnid(txnid.data(), txnid.size());
  td.set_metadata_txnseq(1);
  for (const auto& s : servers) {
    td.add_metadata_servers(s);
  }

  // create metadata file on metadata servers
  eventql::MetadataCoordinator coordinator(cdir_);
  auto rc = coordinator.createFile(
      db_namespace,
      table_name,
      std::move(*metadata_file),
      Vector<String>(servers.begin(), servers.end()));

  if (!rc.isSuccess()) {
    return rc;
  }

  try {
    // create table config
    cdir_->updateTableConfig(td);
    return Status::success();
  } catch (const Exception& e) {
    return Status(e);
  }
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
  if (std::find(primary_key.begin(), primary_key.end(), field_name) !=
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
  if (table.isEmpty() || table.get()->config().deleted()) {
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

Status TableService::dropTable(
    const String& db_namespace,
    const String& table_name) {
  if (!config_->getBool("cluster.allow_drop_table")) {
    return Status(
        eRuntimeError,
        "drop table not allowed (cluster.allow_drop_table=false)");
  }

  auto table = pmap_->findTable(db_namespace, table_name);
  if (table.isEmpty() || table.get()->config().deleted()) {
    return Status(eNotFoundError, "table not found");
  }

  auto td = table.get()->config();
  td.set_deleted(true);
  td.set_generation(td.generation() + 1);

  try {
    cdir_->updateTableConfig(td);
  } catch (const Exception& e) {
    return Status(eRuntimeError, e.getMessage());
  }

  return Status::success();
}

void TableService::listTables(
    const String& tsdb_namespace,
    Function<void (const TableDefinition& table)> fn) const {
  cdir_->listTables([this, tsdb_namespace, fn] (const TableDefinition& td) {
    if (td.customer() != tsdb_namespace) {
      return;
    }

    if (td.deleted()) {
      return;
    }

    fn(td);
  });
}

ReturnCode TableService::insertRecord(
    const String& tsdb_namespace,
    const String& table_name,
    const json::JSONObject::const_iterator& data_begin,
    const json::JSONObject::const_iterator& data_end,
    uint64_t flags /* = 0 */) {
  auto table = pmap_->findTable(tsdb_namespace, table_name);
  if (table.isEmpty() || table.get()->config().deleted()) {
    return ReturnCode::errorf("ENOTFOUND", "table not found: $0", table_name);
  }

  msg::DynamicMessage record(table.get()->schema());
  try {
    record.fromJSON(data_begin, data_end);
  } catch (const std::exception& e) {
    return ReturnCode::exception(e);
  }

  return insertRecord(
      tsdb_namespace,
      table_name,
      record,
      flags);
}

ReturnCode TableService::insertRecord(
    const String& tsdb_namespace,
    const String& table_name,
    const msg::DynamicMessage& data,
    uint64_t flags /* = 0 */) {
  return insertRecords(
      tsdb_namespace,
      table_name,
      &data,
      &data + 1,
      flags);
}

ReturnCode TableService::insertRecords(
    const String& tsdb_namespace,
    const String& table_name,
    const msg::DynamicMessage* begin,
    const msg::DynamicMessage* end,
    uint64_t flags /* = 0 */) {
  MetadataClient metadata_client(cdir_);
  HashMap<SHA1Hash, ShreddedRecordListBuilder> records;
  HashMap<SHA1Hash, Set<String>> servers;

  auto table = pmap_->findTable(tsdb_namespace, table_name);
  if (table.isEmpty() || table.get()->config().deleted()) {
    return ReturnCode::errorf("ENOTFOUND", "table not found: $0", table_name);
  }

  for (auto record = begin; record != end; ++record) {
    // calculate partition key
    auto partition_key_field_name = table.get()->getPartitionKey();
    auto partition_key_field = record->getField(partition_key_field_name);
    if (partition_key_field.isEmpty()) {
      return ReturnCode::errorf(
          "EARG",
          "missing field: $0",
          partition_key_field_name);
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
          return ReturnCode::errorf(
              "EARG",
              "missing field: $0",
              primary_key_columns[0]);
        }
        primary_key = SHA1::compute(f.get());
        break;
      }

      // compound primary key, key value is chained SHA1 of column values
      default: {
        for (const auto& c : primary_key_columns) {
          auto f = record->getField(c);
          if (f.isEmpty()) {
            return ReturnCode::errorf("EARG", "missing field: $0", c);
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
      auto rc = metadata_client.findOrCreatePartition(
          tsdb_namespace,
          table_name,
          encodePartitionKey(
              table.get()->getKeyspaceType(),
              partition_key_field.get()),
          &find_res);

      if (!rc.isSuccess()) {
        return rc;
      }
    }

    SHA1Hash partition_id(
        find_res.partition_id().data(),
        find_res.partition_id().size());

    Set<String> partition_servers(
        find_res.servers_for_insert().begin(),
        find_res.servers_for_insert().end());

    servers[partition_id] = partition_servers;
    auto& record_list_builder = records[partition_id];

    try {
      record_list_builder.addRecordFromProtobuf(
          primary_key,
          WallClock::unixMicros(),
          *record);
    } catch (const std::exception& e) {
      return ReturnCode::exception(e);
    }
  }

  for (auto& p : records) {
    auto rc = insertRecords(
        tsdb_namespace,
        table_name,
        p.first,
        servers[p.first],
        p.second.get());

    if (!rc.isSuccess()) {
      return rc;
    }
  }

  return ReturnCode::success();
}

ReturnCode TableService::insertReplicatedRecords(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    const ShreddedRecordList& records) {
  return insertRecordsLocal(
      tsdb_namespace,
      table_name,
      partition_key,
      records);
}

ReturnCode TableService::insertRecords(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    const Set<String>& servers,
    const ShreddedRecordList& records) {
  size_t nconfirmations = 0;

  /* perform local inserts */
  std::vector<std::string> remote_servers;
  for (const auto& server : servers) {
    if (server == cdir_->getServerID()) {
      logDebug(
          "evqld",
          "Inserting $0 records into evql://localhost/$1/$2/$3",
          records.getNumRecords(),
          tsdb_namespace,
          table_name,
          partition_key.toString());

      auto rc = insertRecordsLocal(
          tsdb_namespace,
          table_name,
          partition_key,
          records);

      if (rc.isSuccess()) {
        ++nconfirmations;
      } else {
        logError("evqld", "Insert failed: $0", rc.getMessage());
      }
    } else {
      logDebug(
          "evqld",
          "Inserting $0 records into evql://$4/$1/$2/$3",
          records.getNumRecords(),
          tsdb_namespace,
          table_name,
          partition_key.toString(),
          server);

      remote_servers.emplace_back(server);
    }
  }

  /* build rpc payload */
  std::string rpc_payload;
  {
    std::string rpc_body;
    auto rpc_body_os = StringOutputStream::fromString(&rpc_body);
    records.encode(rpc_body_os.get());

    native_transport::ReplInsertFrame i_frame;
    i_frame.setDatabase(tsdb_namespace);
    i_frame.setTable(table_name);
    i_frame.setPartitionID(partition_key.toString());
    i_frame.setBody(rpc_body);

    auto rpc_payload_os = StringOutputStream::fromString(&rpc_payload);
    i_frame.writeTo(rpc_payload_os.get());
  }

  /* execute rpcs */
  native_transport::TCPAsyncClient rpc_client(
      config_,
      cdir_,
      remote_servers.size(), /* max_concurrent_tasks */
      1,                     /* max_concurrent_tasks_per_host */
      true);                 /* tolerate failures */

  rpc_client.setResultCallback([&nconfirmations] (
      void* priv,
      uint16_t opcode,
      uint16_t flags,
      const char* payload,
      size_t payload_size) -> ReturnCode {
    switch (opcode) {
      case EVQL_OP_ACK:
        ++nconfirmations;
        return ReturnCode::success();
      default:
        return ReturnCode::error("ERUNTIME", "unexpected opcode");
    }
  });

  for (const auto& s : remote_servers) {
    rpc_client.addRPC(EVQL_OP_REPL_INSERT, 0, std::string(rpc_payload), { s });
  }

  auto rc = rpc_client.execute();
  if (!rc.isSuccess()) {
    return rc;
  }

  /* check if at least N inserts were successful */
  if (nconfirmations >= 1) { // FIXME min consistency level
    return ReturnCode::success();
  } else {
    return ReturnCode::error("ERUNTIME", "not enough live servers for insert");
  }
}

ReturnCode TableService::insertRecordsLocal(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    const ShreddedRecordList& records) try {
  auto partition = pmap_->findOrCreatePartition(
      tsdb_namespace,
      table_name,
      partition_key);

  auto writer = partition->getWriter();
  auto inserted_ids = writer->insertRecords(records);

  if (!inserted_ids.empty()) {
    auto change = mkRef(new PartitionChangeNotification());
    change->partition = partition;
    pmap_->publishPartitionChange(change);
  }

  return ReturnCode::success();
} catch (const std::exception& e) {
  return ReturnCode::exception(e);
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
  if (writer->compact(true)) {
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

Option<RefPtr<msg::MessageSchema>> TableService::tableSchema(
    const String& tsdb_namespace,
    const String& table_key) {
  auto table = pmap_->findTable(
      tsdb_namespace,
      table_key);

  if (table.isEmpty() || table.get()->config().deleted()) {
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

  if (table.isEmpty() || table.get()->config().deleted()) {
    return None<TableDefinition>();
  } else {
    return Some(table.get()->config());
  }
}

} // namespace tdsb

