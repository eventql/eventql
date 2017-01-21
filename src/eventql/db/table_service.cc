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
#include <eventql/util/logging.h>
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

Status setTableProperty(
    TableConfig* config,
    std::pair<std::string, std::string> property);

TableService::TableService(DatabaseContext* dbctx) : dbctx_(dbctx) {}

Status TableService::createDatabase(const String& db_name) {
  if (!dbctx_->config->getBool("cluster.allow_create_database")) {
    return Status(eRuntimeError, "create database not allowed");
  }

  try {
    auto c = dbctx_->config_directory->getNamespaceConfig(db_name);
    return Status(eRuntimeError, "database already exists");
  } catch (const std::exception& e) {
    /* fallthrough */
  }

  NamespaceConfig cfg;
  cfg.set_customer(db_name);
  try {
    dbctx_->config_directory->updateNamespaceConfig(cfg);
  } catch (const std::exception& e) {
    return Status(e);
  }

  return Status::success();

}

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
  auto old_table = dbctx_->partition_map->findTable(db_namespace, table_name);
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

    if (p.first == "user_defined_partitions" && p.second == "true") {
      tblcfg->set_enable_user_defined_partitions(true);
      continue;
    }

    auto rc = setTableProperty(tblcfg, p);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  // check preconditions
  if (tblcfg->enable_finite_partitions() &&
      tblcfg->enable_user_defined_partitions()) {
    return Status(
        eIllegalArgumentError,
        "partition_size_hint and user_defined_partitions are mutually "
        "exclusive");
  }

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

  auto cconf = dbctx_->config_directory->getClusterConfig();

  // generate new metadata file
  std::vector<String> servers;
  {
    auto rc = dbctx_->server_alloc->allocateServers(
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
  } else if (tblcfg->enable_user_defined_partitions()) {
    metadata_file.reset(
        new MetadataFile(
            txnid,
            1,
            keyspace_type,
            { },
            MFILE_USERDEFINED));
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
  auto rc = dbctx_->metadata_coordinator->createFile(
      db_namespace,
      table_name,
      std::move(*metadata_file),
      Vector<String>(servers.begin(), servers.end()));

  if (!rc.isSuccess()) {
    return rc;
  }

  try {
    // create table config
    dbctx_->config_directory->updateTableConfig(td);
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

Status setTableProperty(
    TableConfig* config,
    std::pair<std::string, std::string> property) {
  if (property.first == "write_consistency_level") {
    EVQL_CLEVEL_WRITE clevel;
    auto rc = writeConsistencyLevelFromString(property.second, &clevel);
    if (rc.isSuccess()) {
      config->set_default_write_consistency_level(clevel);
      return Status::success();
    } else {
      return Status(rc);
    }
  }

  if (property.first == "partition_split_threshold") {
    if (StringUtil::isNumber(property.second)) {
      try {
        auto value = std::stoull(property.second);
        config->set_override_partition_split_threshold(value);
        return Status::success();

      } catch (const std::exception& e) {
        return Status(e);
      }
    } else {
      return Status(
          eRuntimeError,
          StringUtil::format("can't convert $0 to uint64", property.second));
    }
  }

  if (property.first == "enable_async_split") {
    auto value = property.second;
    StringUtil::toLower(&value);
    if (value == "true") {
      config->set_enable_async_split(true);
      return Status::success();

    } else if (value == "false") {
      config->set_enable_async_split(false);
      return Status::success();

    } else {
        return Status(
            eRuntimeError,
            StringUtil::format("can't convert $0 to bool", property.second));
    }
  }

  if (property.first == "disable_replication") {
    auto value = property.second;
    StringUtil::toLower(&value);
    if (value == "true") {
      config->set_disable_replication(true);
      return Status::success();

    } else if (value == "false") {
      config->set_disable_replication(false);
      return Status::success();

    } else {
        return Status(
            eRuntimeError,
            StringUtil::format("can't convert $0 to bool", property.second));
    }
  }

  return Status(
      eRuntimeError,
      StringUtil::format("unknown property $0", property.first));
}

Status TableService::alterTable(
    const String& db_namespace,
    const String& table_name,
    Vector<TableService::AlterTableOperation> operations,
    const std::vector<std::pair<std::string, std::string>>& properties /* = {} */) {
  auto table = dbctx_->partition_map->findTable(db_namespace, table_name);
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

  for (const auto& p : properties) {
    auto rc = setTableProperty(td.mutable_config(), p);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  try {
    dbctx_->config_directory->updateTableConfig(td);
  } catch (const Exception& e) {
    return Status(eRuntimeError, e.getMessage());
  }

  return Status::success();
}

Status TableService::dropTable(
    const String& db_namespace,
    const String& table_name) {
  if (!dbctx_->config->getBool("cluster.allow_drop_table")) {
    return Status(
        eRuntimeError,
        "drop table not allowed (cluster.allow_drop_table=false)");
  }

  auto table = dbctx_->partition_map->findTable(db_namespace, table_name);
  if (table.isEmpty() || table.get()->config().deleted()) {
    return Status(eNotFoundError, "table not found");
  }

  auto td = table.get()->config();
  td.set_deleted(true);
  td.set_generation(td.generation() + 1);

  try {
    dbctx_->config_directory->updateTableConfig(td);
  } catch (const Exception& e) {
    return Status(eRuntimeError, e.getMessage());
  }

  return Status::success();
}

void TableService::listTables(
    const String& tsdb_namespace,
    Function<void (const TableDefinition& table)> fn) const {
  dbctx_->config_directory->listTables([this, tsdb_namespace, fn] (const TableDefinition& td) {
    if (td.customer() != tsdb_namespace) {
      return;
    }

    if (td.deleted()) {
      return;
    }

    fn(td);
  });
}

Status TableService::listPartitions(
    const String& db_namespace,
    const String& table_name,
    Function<void (const TablePartitionInfo& partition)> fn) const {
  auto table = dbctx_->partition_map->findTable(db_namespace, table_name);

  RefPtr<MetadataFile> metadata_file;
  auto rc = dbctx_->metadata_client->fetchLatestMetadataFile(
      db_namespace,
      table_name,
      &metadata_file);

  if (!rc.isSuccess()) {
    return rc;
  }

  auto partition_map = metadata_file->getPartitionMap();
  for (size_t i = 0; i < partition_map.size(); ++i) {
    const auto& e = partition_map[i];

    TablePartitionInfo p_info;
    p_info.partition_id = e.partition_id.toString();

    for (const auto& s : e.servers) {
      p_info.server_ids.emplace_back(s.server_id);
    }

    switch (metadata_file->getKeyspaceType()) {
      case KEYSPACE_UINT64: {
        uint64_t keyrange_uint = 0;
        if (!e.begin.empty()) {
          assert(e.begin.size() == sizeof(uint64_t));
          memcpy((char*) &keyrange_uint, e.begin.data(), sizeof(uint64_t));
        }

        p_info.keyrange_begin = StringUtil::format(
            "$0 [$1]",
            UnixTime(keyrange_uint),
            keyrange_uint / kMicrosPerSecond);
        break;
      }
      case KEYSPACE_STRING: {
        p_info.keyrange_begin = e.begin;
        break;
      }
    }

    if (e.splitting) {
      Set<String> servers_low;
      for (const auto& s : e.split_servers_low) {
        servers_low.emplace(s.server_id);
      }
      Set<String> servers_high;
      for (const auto& s : e.split_servers_high) {
        servers_high.emplace(s.server_id);
      }

      p_info.extra_info += StringUtil::format(
          "SPLITTING @ $0 into $1 on $2, $3 on $4",
          decodePartitionKey(table.get()->getKeyspaceType(), e.split_point),
          e.split_partition_id_low,
          inspect(servers_low),
          e.split_partition_id_high,
          inspect(servers_high));
    }

    std::string keyrange_end;
    if (metadata_file->hasFinitePartitions()) {
      keyrange_end = e.end;
    } else {
      if (i < partition_map.size() - 1) {
        keyrange_end = partition_map[i + 1].begin;
      }
    }

    switch (metadata_file->getKeyspaceType()) {
      case KEYSPACE_UINT64: {
        uint64_t keyrange_uint = 0;
        if (!keyrange_end.empty()) {
          assert(keyrange_end.size() == sizeof(uint64_t));
          memcpy((char*) &keyrange_uint, keyrange_end.data(), sizeof(uint64_t));
        }

        p_info.keyrange_end = StringUtil::format(
            "$0 [$1]",
            UnixTime(keyrange_uint),
            keyrange_uint / kMicrosPerSecond);
        break;
      }
      case KEYSPACE_STRING: {
        p_info.keyrange_end = keyrange_end;
        break;
      }
    }

    fn(p_info);
  }

  return Status::success();
}

ReturnCode TableService::insertRecord(
    const String& tsdb_namespace,
    const String& table_name,
    const json::JSONObject::const_iterator& data_begin,
    const json::JSONObject::const_iterator& data_end,
    Option<EVQL_CLEVEL_WRITE> consistency_level /* = None<EVQL_CLEVEL_WRITE> */,
    uint64_t flags /* = 0 */) {
  auto table = dbctx_->partition_map->findTable(tsdb_namespace, table_name);
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
      consistency_level,
      flags);
}

ReturnCode TableService::insertRecords(
    const String& tsdb_namespace,
    const String& table_name,
    const json::JSONObject* begin,
    const json::JSONObject* end,
    Option<EVQL_CLEVEL_WRITE> consistency_level /* = None<EVQL_CLEVEL_WRITE> */,
    uint64_t flags /* = 0 */) {
  auto table = dbctx_->partition_map->findTable(tsdb_namespace, table_name);
  if (table.isEmpty() || table.get()->config().deleted()) {
    return ReturnCode::errorf("ENOTFOUND", "table not found: $0", table_name);
  }

  std::vector<msg::DynamicMessage> records;
  for (auto iter = begin; iter != end; ++iter) {
    try {
      records.emplace_back(table.get()->schema());
      records.back().fromJSON(iter->begin(), iter->end());
    } catch (const std::exception& e) {
      return ReturnCode::exception(e);
    }
  }

  return insertRecords(
      tsdb_namespace,
      table_name,
      &*records.begin(),
      &*records.end(),
      consistency_level,
      flags);
}

ReturnCode TableService::insertRecord(
    const String& tsdb_namespace,
    const String& table_name,
    const msg::DynamicMessage& data,
    Option<EVQL_CLEVEL_WRITE> consistency_level /* = None<EVQL_CLEVEL_WRITE> */,
    uint64_t flags /* = 0 */) {
  return insertRecords(
      tsdb_namespace,
      table_name,
      &data,
      &data + 1,
      consistency_level,
      flags);
}

ReturnCode TableService::insertRecords(
    const String& tsdb_namespace,
    const String& table_name,
    const msg::DynamicMessage* begin,
    const msg::DynamicMessage* end,
    Option<EVQL_CLEVEL_WRITE> consistency_level /* = None<EVQL_CLEVEL_WRITE> */,
    uint64_t flags /* = 0 */) {
  auto table = dbctx_->partition_map->findTable(tsdb_namespace, table_name);
  if (table.isEmpty() || table.get()->config().deleted()) {
    return ReturnCode::errorf("ENOTFOUND", "table not found: $0", table_name);
  }

  auto ks = table.get()->getKeyspaceType();

  /* if no consistency level specified, set default level */
  if (consistency_level.isEmpty()) {
    consistency_level = Some(table.get()->getDefaultWriteConsistencyLevel());
  }

  /* group inserts into partitions */
  HashMap<std::string, ShreddedRecordListBuilder> records;
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

    auto partition_key = encodePartitionKey(
        table.get()->getKeyspaceType(),
        partition_key_field.get());

    auto& record_list_builder = records[partition_key];
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
    const auto& partition_key = p.first;

    /* lookup partition targets */
    PartitionFindResponse find_res;
    {
      auto rc = dbctx_->metadata_client->findPartition(
          tsdb_namespace,
          table_name,
          partition_key,
          true, /* allow create */
          &find_res);

      if (!rc.isSuccess()) {
        return rc;
      }
    }

    std::vector<PartitionWriteTarget> targets;
    for (const auto& t : find_res.write_targets()) {
      if (t.strict_only()) {
        switch (consistency_level.get()) {
          case EVQL_CLEVEL_WRITE_STRICT:
            break;
          case EVQL_CLEVEL_WRITE_RELAXED:
          case EVQL_CLEVEL_WRITE_BEST_EFFORT:
            continue;
        }
      }

      if (t.has_keyrange_begin() &&
          t.keyrange_begin().size() > 0 &&
          comparePartitionKeys(ks, partition_key, t.keyrange_begin()) < 0) {
        continue;
      }

      if (t.has_keyrange_end() &&
          t.keyrange_end().size() > 0 &&
          comparePartitionKeys(ks, partition_key, t.keyrange_end()) >= 0) {
        continue;
      }

      targets.emplace_back(t);
    }

    /* perform insert into all targets */
    {
      auto rc = insertRecords(
          tsdb_namespace,
          table_name,
          targets,
          p.second.get(),
          consistency_level.get());

      if (!rc.isSuccess()) {
        return rc;
      }
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
    const std::vector<PartitionWriteTarget>& targets,
    const ShreddedRecordList& records,
    EVQL_CLEVEL_WRITE consistency_level) {
  size_t nconfirmations = 0;

  /* perform local inserts */
  std::vector<PartitionWriteTarget> remote_targets;
  for (const auto& t : targets) {
    SHA1Hash partition_id(t.partition_id().data(), t.partition_id().size());

    if (t.server_id() == dbctx_->config_directory->getServerID()) {
      logDebug(
          "evqld",
          "Inserting $0 records into evql://localhost/$1/$2/$3",
          records.getNumRecords(),
          tsdb_namespace,
          table_name,
          partition_id.toString());

      auto rc = insertRecordsLocal(
          tsdb_namespace,
          table_name,
          partition_id,
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
          partition_id.toString(),
          t.server_id());

      remote_targets.emplace_back(t);
    }
  }

  /* build rpc body */
  std::string rpc_body;
  auto rpc_body_os = StringOutputStream::fromString(&rpc_body);
  records.encode(rpc_body_os.get());

  /* execute rpcs */
  native_transport::TCPAsyncClient rpc_client(
      dbctx_->config,
      dbctx_->config_directory,
      dbctx_->connection_pool,
      dbctx_->dns_cache,
      remote_targets.size(), /* max_concurrent_tasks */
      1,                     /* max_concurrent_tasks_per_host */
      true);                 /* tolerate failures */

  rpc_client.setResultCallback([&nconfirmations] (
      void* priv,
      uint16_t opcode,
      uint16_t flags,
      const char* payload,
      size_t payload_size) -> ReturnCode {
    switch (opcode) {
      case EVQL_OP_ACK: {
        ++nconfirmations;
        return ReturnCode::success();
      }
      default:
        return ReturnCode::error("ERUNTIME", "unexpected opcode");
    }
  });

  for (const auto& t : remote_targets) {
    SHA1Hash partition_id(t.partition_id().data(), t.partition_id().size());

    native_transport::ReplInsertFrame i_frame;
    i_frame.setDatabase(tsdb_namespace);
    i_frame.setTable(table_name);
    i_frame.setPartitionID(partition_id.toString());
    i_frame.setBody(rpc_body);

    std::string rpc_payload;
    auto rpc_payload_os = StringOutputStream::fromString(&rpc_payload);
    i_frame.writeTo(rpc_payload_os.get());

    rpc_client.addRPC(
        EVQL_OP_REPL_INSERT,
        0,
        std::move(rpc_payload),
        { t.server_id() });
  }

  auto rc = rpc_client.execute();
  if (!rc.isSuccess()) {
    return rc;
  }

  /* check if at least N inserts were successful */
  size_t required_confirmations = 1;
  switch (consistency_level) {
    case EVQL_CLEVEL_WRITE_STRICT:
    case EVQL_CLEVEL_WRITE_RELAXED:
      required_confirmations = (targets.size() + 1) / 2;
      break;
    case EVQL_CLEVEL_WRITE_BEST_EFFORT:
      required_confirmations = 1;
      break;
  }

  if (nconfirmations >= required_confirmations) {
    return ReturnCode::success();
  } else {
    return ReturnCode::error(
        "ERUNTIME",
        "couldn't perform enough replica writes for the requested consistency "
        "level; only $0 out of $1 (required) writes succeeded",
        nconfirmations,
        required_confirmations);
  }
}

ReturnCode TableService::insertRecordsLocal(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    const ShreddedRecordList& records) try {
  auto partition = dbctx_->partition_map->findOrCreatePartition(
      tsdb_namespace,
      table_name,
      partition_key);

  auto writer = partition->getWriter();
  auto inserted_ids = writer->insertRecords(records);

  if (!inserted_ids.empty()) {
    auto change = mkRef(new PartitionChangeNotification());
    change->partition = partition;
    dbctx_->partition_map->publishPartitionChange(change);
  }

  return ReturnCode::success();
} catch (const std::exception& e) {
  return ReturnCode::exception(e);
}

void TableService::compactPartition(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key) {
  auto partition = dbctx_->partition_map->findOrCreatePartition(
      tsdb_namespace,
      table_name,
      partition_key);

  auto writer = partition->getWriter();
  if (writer->compact(true)) {
    auto change = mkRef(new PartitionChangeNotification());
    change->partition = partition;
    dbctx_->partition_map->publishPartitionChange(change);
  }
}

void TableService::commitPartition(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key) {
  auto partition = dbctx_->partition_map->findOrCreatePartition(
      tsdb_namespace,
      table_name,
      partition_key);

  auto writer = partition->getWriter();
  if (writer->commit()) {
    auto change = mkRef(new PartitionChangeNotification());
    change->partition = partition;
    dbctx_->partition_map->publishPartitionChange(change);
  }
}

Option<RefPtr<msg::MessageSchema>> TableService::tableSchema(
    const String& tsdb_namespace,
    const String& table_key) {
  auto table = dbctx_->partition_map->findTable(
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
  auto table = dbctx_->partition_map->findTable(
      tsdb_namespace,
      table_key);

  if (table.isEmpty() || table.get()->config().deleted()) {
    return None<TableDefinition>();
  } else {
    return Some(table.get()->config());
  }
}

} // namespace tdsb

