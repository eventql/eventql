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
#include <eventql/sql/CSTableScan.h>
#include <eventql/util/json/json.h>

namespace eventql {

TSDBTableProvider::TSDBTableProvider(
    const String& tsdb_namespace,
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    TableService* table_service,
    InternalAuth* auth) :
    tsdb_namespace_(tsdb_namespace),
    partition_map_(partition_map),
    replication_scheme_(replication_scheme),
    table_service_(table_service),
    auth_(auth) {}

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
  Vector<SHA1Hash> partitions;
  if (table_ref.partition_key.isEmpty()) {
    partitions = partitioner->listPartitions(seqscan->constraints());
  } else {
    partitions.emplace_back(table_ref.partition_key.get());
  }

  return Option<ScopedPtr<csql::TableExpression>>(
      mkScoped(
          new TableScan(
              ctx,
              execution_context,
              tsdb_namespace_,
              table_ref.table_key,
              partitions,
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
  { "FLOAT", msg::FieldType::DOUBLE }
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
