/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#include "eventql/eventql.h"
#include <eventql/util/SHA1.h>
#include <eventql/server/sql/table_provider.h>
#include <eventql/db/table_service.h>
#include <eventql/db/metadata_client.h>
#include <eventql/sql/CSTableScan.h>
#include <eventql/util/json/json.h>

namespace eventql {

TSDBTableProvider::TSDBTableProvider(
    const String& tsdb_namespace,
    PartitionMap* partition_map,
    ConfigDirectory* cdir,
    ReplicationScheme* replication_scheme,
    TableService* table_service,
    InternalAuth* auth) :
    tsdb_namespace_(tsdb_namespace),
    partition_map_(partition_map),
    cdir_(cdir),
    replication_scheme_(replication_scheme),
    table_service_(table_service),
    auth_(auth) {}

KeyRange TSDBTableProvider::findKeyRange(
    const String& partition_key,
    const Vector<csql::ScanConstraint>& constraints) {
  uint64_t lower_limit;
  bool has_lower_limit = false;
  uint64_t upper_limit;
  bool has_upper_limit = false;

  for (const auto& c : constraints) {
    if (c.column_name != partition_key) {
      continue;
    }

    uint64_t val;
    try {
      val = c.value.getInteger();
    } catch (const StandardException& e) {
      continue;
    }

    switch (c.type) {
      case csql::ScanConstraintType::EQUAL_TO:
        lower_limit = val;
        upper_limit = val;
        has_lower_limit = true;
        has_upper_limit = true;
        break;
      case csql::ScanConstraintType::NOT_EQUAL_TO:
        break;
      case csql::ScanConstraintType::LESS_THAN:
        upper_limit = val - 1;
        has_upper_limit = true;
        break;
      case csql::ScanConstraintType::LESS_THAN_OR_EQUAL_TO:
        upper_limit = val;
        has_upper_limit = true;
        break;
      case csql::ScanConstraintType::GREATER_THAN:
        lower_limit = val + 1;
        has_lower_limit = true;
        break;
      case csql::ScanConstraintType::GREATER_THAN_OR_EQUAL_TO:
        lower_limit = val;
        has_lower_limit = true;
        break;
    }
  }

  KeyRange kr;
  if (has_lower_limit) {
    kr.begin = String((const char*) &lower_limit, sizeof(lower_limit));
  }

  if (has_upper_limit) {
    kr.end = String((const char*) &upper_limit, sizeof(upper_limit));
  }

  return kr;
}

Option<ScopedPtr<csql::TableExpression>> TSDBTableProvider::buildSequentialScan(
    csql::Transaction* ctx,
    csql::ExecutionContext* execution_context,
    RefPtr<csql::SequentialScanNode> seqscan) const {
  auto table_ref = TSDBTableRef::parse(seqscan->tableName());
  if (partition_map_->findTable(tsdb_namespace_, table_ref.table_key).isEmpty()) {
    return None<ScopedPtr<csql::TableExpression>>();
  }

  auto table = partition_map_->findTable(tsdb_namespace_, table_ref.table_key);
  if (table.isEmpty()) {
    return None<ScopedPtr<csql::TableExpression>>();
  }

  auto partitioner = table.get()->partitioner();
  Set<SHA1Hash> partitions;
  if (table_ref.partition_key.isEmpty()) {
    if (table.get()->partitionerType() == TBL_PARTITION_TIMEWINDOW) {
      auto keyrange = findKeyRange(
          table.get()->config().config().partition_key(),
          seqscan->constraints());

      MetadataClient metadata_client(cdir_);
      PartitionListResponse partition_list;
      auto rc = metadata_client.listPartitions(
          tsdb_namespace_,
          table_ref.table_key,
          keyrange,
          &partition_list);

      if (!rc.isSuccess()) {
        RAISEF(kRuntimeError, "metadata lookup failure: $0", rc.message());
      }

      for (const auto& p : partition_list.partitions()) {
        partitions.emplace(
            SHA1Hash(p.partition_id().data(), p.partition_id().size()));
      }
    } else {
      auto partitioner = table.get()->partitioner();
      for (const auto& p : partitioner->listPartitions(seqscan->constraints())) {
        partitions.emplace(p);
      }
    }
  } else {
    partitions.emplace(table_ref.partition_key.get());
  }

  return Option<ScopedPtr<csql::TableExpression>>(
      mkScoped(
          new TableScan(
              ctx,
              execution_context,
              tsdb_namespace_,
              table_ref.table_key,
              Vector<SHA1Hash>(partitions.begin(), partitions.end()),
              seqscan,
              partition_map_,
              replication_scheme_,
              auth_)));
}

static HashMap<String, msg::FieldType> kTypeMap = {
  { "STRING", msg::FieldType::STRING },
  { "BOOL", msg::FieldType::BOOLEAN },
  { "BOOLEAN", msg::FieldType::BOOLEAN },
  { "DATETIME", msg::FieldType::DATETIME },
  { "TIME", msg::FieldType::DATETIME },
  { "UINT32", msg::FieldType::UINT32 },
  { "UINT64", msg::FieldType::UINT64 },
  { "DOUBLE", msg::FieldType::DOUBLE },
  { "FLOAT", msg::FieldType::DOUBLE },
  { "RECORD", msg::FieldType::OBJECT }
};

static Status buildMessageSchema(
    const csql::TableSchema::ColumnList& columns,
    msg::MessageSchema* schema) {
  uint32_t id = 0;
  Set<String> column_names;

  for (const auto& c : columns) {
    if (column_names.count(c->column_name) > 0) {
      return Status(
          eIllegalArgumentError,
          StringUtil::format("duplicate column: $0", c->column_name));
    }

    column_names.emplace(c->column_name);

    bool repeated = false;
    bool optional = true;

    for (const auto& o : c->column_options) {
      switch (o) {
        case csql::TableSchema::ColumnOptions::NOT_NULL:
          optional = false;
          break;
        case csql::TableSchema::ColumnOptions::REPEATED:
          repeated = true;
          break;
        default:
          continue;
      }
    }

    switch (c->column_class) {
      case csql::TableSchema::ColumnClass::SCALAR: {
        auto type_str = c->column_type;
        StringUtil::toUpper(&type_str);
        auto type = kTypeMap.find(type_str);
        if (type == kTypeMap.end()) {
          return Status(
              eIllegalArgumentError,
              StringUtil::format(
                  "invalid type: '$0' for column '$1'",
                  c->column_type,
                  c->column_name));
        }

        schema->addField(
            msg::MessageSchemaField(
                ++id,
                c->column_name,
                type->second,
                0, /* type size */
                repeated,
                optional));

        break;
      }

      case csql::TableSchema::ColumnClass::RECORD: {
        auto s = mkRef(new msg::MessageSchema(nullptr));

        auto rc = buildMessageSchema(c->getSubColumns(), s.get());
        if (!rc.isSuccess()) {
          return rc;
        }

        schema->addField(
            msg::MessageSchemaField::mkObjectField(
                ++id,
                c->column_name,
                repeated,
                optional,
                s));

        break;
      }

    }
  }

  return Status::success();
}

Status TSDBTableProvider::createTable(
    const csql::CreateTableNode& create_table) {
  auto primary_key = create_table.getPrimaryKey();
  auto table_schema = create_table.getTableSchema();
  auto msg_schema = mkRef(new msg::MessageSchema(nullptr));
  auto rc = buildMessageSchema(table_schema.getColumns(), msg_schema.get());
  if (!rc.isSuccess()) {
    return rc;
  }

  return table_service_->createTable(
      tsdb_namespace_,
      create_table.getTableName(),
      *msg_schema,
      primary_key);
}

Status TSDBTableProvider::alterTable(const csql::AlterTableNode& alter_table) {
  auto operations = alter_table.getOperations();
  for (auto o : operations) {
    if (o.optype == csql::AlterTableNode::AlterTableOperationType::OP_ADD_COLUMN) {
      auto type_str = o.column_type;
      StringUtil::toUpper(&type_str);
      auto type = kTypeMap.find(type_str);
      if (type == kTypeMap.end()) {
        return Status(
            eIllegalArgumentError,
            StringUtil::format(
                "invalid type: '$0' for column '$1'",
                o.column_type,
                o.column_name));
      }

      auto rc = table_service_->addColumn(
          tsdb_namespace_,
          alter_table.getTableName(),
          o.column_name,
          type->second,
          o.is_repeated,
          o.is_optional);

      if (!rc.isSuccess()) {
        return rc;
      }
    } else {
      auto rc = table_service_->removeColumn(
          tsdb_namespace_,
          alter_table.getTableName(),
          o.column_name);
      if (!rc.isSuccess()) {
        return rc;
      }
    }
  }

  return Status::success();
}

Status TSDBTableProvider::insertRecord(
    const String& table_name,
    Vector<Pair<String, csql::SValue>> data) {

  auto schema = table_service_->tableSchema(tsdb_namespace_, table_name);
  if (schema.isEmpty()) {
    return Status(eRuntimeError, "table not found");
  }

  auto msg = new msg::DynamicMessage(schema.get());
  for (auto e : data) {
    if (!msg->addField(e.first, e.second.getString())) {
      return Status(
          eRuntimeError,
          StringUtil::format("field not found: $0", e.first)); //FIXME better error msg
    }
  }

  try {
    table_service_->insertRecord(
        tsdb_namespace_,
        table_name,
        *msg);

  } catch (const Exception& e) {
    return Status(eRuntimeError, e.getMessage());
  }

  return Status::success();
}

Status TSDBTableProvider::insertRecord(
    const String& table_name,
    const String& json_str) {

  auto json = json::parseJSON(json_str);
  try {
    table_service_->insertRecord(
        tsdb_namespace_,
        table_name,
        json.begin(),
        json.end());
  } catch (const Exception& e) {
    return Status(eRuntimeError, e.getMessage());
  }

  return Status::success();
}

void TSDBTableProvider::listTables(
    Function<void (const csql::TableInfo& table)> fn) const {
  partition_map_->listTables(
      tsdb_namespace_,
      [this, fn] (const TSDBTableInfo& table) {
    fn(tableInfoForTable(table));
  });
}

Option<csql::TableInfo> TSDBTableProvider::describe(
    const String& table_name) const {
  auto table_ref = TSDBTableRef::parse(table_name);

  auto table = partition_map_->tableInfo(tsdb_namespace_, table_ref.table_key);
  if (table.isEmpty()) {
    return None<csql::TableInfo>();
  } else {
    auto tblinfo = tableInfoForTable(table.get());
    tblinfo.table_name = table_name;
    return Some(tblinfo);
  }
}

csql::TableInfo TSDBTableProvider::tableInfoForTable(
    const TSDBTableInfo& table) const {
  csql::TableInfo ti;
  ti.table_name = table.table_name;

  for (const auto& tag : table.config.tags()) {
    ti.tags.insert(tag);
  }

  for (const auto& col : table.schema->columns()) {
    csql::ColumnInfo ci;
    ci.column_name = col.first;
    ci.type = col.second.typeName();
    ci.type_size = col.second.typeSize();
    ci.is_nullable = col.second.optional;

    ti.columns.emplace_back(ci);
  }

  return ti;
}

const String& TSDBTableProvider::getNamespace() const {
  return tsdb_namespace_;
}

} // namespace csql
