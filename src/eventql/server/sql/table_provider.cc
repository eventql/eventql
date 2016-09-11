/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <algorithm>
#include <eventql/util/SHA1.h>
#include <eventql/server/sql/table_provider.h>
#include <eventql/db/table_service.h>
#include <eventql/sql/CSTableScan.h>
#include <eventql/sql/qtree/QueryTreeUtil.h>
#include <eventql/util/json/json.h>

namespace eventql {

TSDBTableProvider::TSDBTableProvider(
    const String& tsdb_namespace,
    PartitionMap* partition_map,
    ConfigDirectory* cdir,
    TableService* table_service,
    InternalAuth* auth) :
    tsdb_namespace_(tsdb_namespace),
    partition_map_(partition_map),
    cdir_(cdir),
    table_service_(table_service),
    auth_(auth) {}

KeyRange TSDBTableProvider::findKeyRange(
    KeyspaceType keyspace,
    const String& partition_key,
    const Vector<csql::ScanConstraint>& constraints) {
  String lower_limit;
  bool has_lower_limit = false;
  String upper_limit;
  bool has_upper_limit = false;

  for (const auto& c : constraints) {
    if (c.column_name != partition_key) {
      continue;
    }

    String val;
    try {
      val = encodePartitionKey(keyspace, c.value.getString());
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
        upper_limit = val; // FIXME should be predecessor(val)
        has_upper_limit = true;
        break;
      case csql::ScanConstraintType::LESS_THAN_OR_EQUAL_TO:
        upper_limit = val;
        has_upper_limit = true;
        break;
      case csql::ScanConstraintType::GREATER_THAN:
        lower_limit = val; // FIXME should be sucessor(val)
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
    kr.begin = lower_limit;
  }

  if (has_upper_limit) {
    kr.end = upper_limit;
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

  {
    auto table_schema = table.get()->schema();
    Set<String> table_schema_columns;
    for (const auto& c : table_schema->columns()) {
      table_schema_columns.insert(c.first);
    }

    auto check_columns = [&table_schema_columns] (
        const RefPtr<csql::ColumnReferenceNode>& col) {
      auto colname = col->columnName();
      if (table_schema_columns.count(colname) == 0) {
        RAISEF(kNotFoundError, "column(s) not found: $0", colname);
      }
    };

    for (const auto& e : seqscan->selectList()) {
      csql::QueryTreeUtil::findColumns(e->expression(), check_columns);
    }

    if (!seqscan->whereExpression().isEmpty()) {
      csql::QueryTreeUtil::findColumns(
          seqscan->whereExpression().get().get(),
          check_columns);
    }
  }

  Vector<TableScan::PartitionLocation> partitions;
  if (table_ref.partition_key.isEmpty()) {
    auto keyrange = findKeyRange(
        table.get()->getKeyspaceType(),
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
      TableScan::PartitionLocation pl;
      pl.partition_id = SHA1Hash(
          p.partition_id().data(),
          p.partition_id().size());

      pl.qtree = seqscan->deepCopy().asInstanceOf<csql::SequentialScanNode>();

      if (!pl.qtree->whereExpression().isEmpty()) {
        auto where_expr = seqscan->whereExpression().get();
        where_expr = simplifyWhereExpression(
            table.get(),
            p.keyrange_begin(),
            p.keyrange_end(),
            where_expr);

        pl.qtree->setWhereExpression(where_expr);
      }

      for (const auto& s : p.servers()) {
        auto server_cfg = cdir_->getServerConfig(s);
        if (server_cfg.server_status() != SERVER_UP) {
          continue;
        }

        ReplicaRef rref(SHA1::compute(s), server_cfg.server_addr());
        rref.name = s;
        pl.servers.emplace_back(rref);
      }

      partitions.emplace_back(pl);
    }
  } else {
    TableScan::PartitionLocation pl;
    pl.partition_id = table_ref.partition_key.get();
    pl.qtree = seqscan;

    auto partition = partition_map_->findPartition(
        tsdb_namespace_,
        table_ref.table_key,
        pl.partition_id);

    if (!pl.qtree->whereExpression().isEmpty() &&
        !partition.isEmpty()) {
      const auto& pstate = partition.get()->getSnapshot()->state;

      auto where_expr = pl.qtree->whereExpression().get();
      where_expr = simplifyWhereExpression(
          table.get(),
          pstate.partition_keyrange_begin(),
          pstate.partition_keyrange_end(),
          where_expr);

      pl.qtree->setWhereExpression(where_expr);
    }

    partitions.emplace_back(pl);
  }

  Option<SHA1Hash> cache_key;
  for (const auto& p : partitions) {
    if (!p.servers.empty()) { // FIXME better check if local partition
      cache_key = None<SHA1Hash>();
      break;
    }

    auto partition = partition_map_->findPartition(
          tsdb_namespace_,
          table_ref.table_key,
          p.partition_id);

    uint64_t lsm_sequence = 0;
    if (!partition.isEmpty()) {
      lsm_sequence = partition.get()->getSnapshot()->state.lsm_sequence();
    }

    SHA1Hash expression_fingerprint;
    for (const auto& slnode : p.qtree->selectList()) {
      expression_fingerprint = SHA1::compute(
          expression_fingerprint.toString() + slnode->toString());
    }

    if (!p.qtree->whereExpression().isEmpty()) {
      expression_fingerprint = SHA1::compute(
          expression_fingerprint.toString() +
          p.qtree->whereExpression().get()->toString());
    }

    cache_key = Some(
        SHA1::compute(
            StringUtil::format(
                "$0~$1~$2~$3~$4~$5",
                cache_key.isEmpty() ? "" : cache_key.get().toString(),
                expression_fingerprint.toString(),
                tsdb_namespace_,
                table_ref.table_key,
                p.partition_id,
                lsm_sequence)));
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
              cache_key,
              partition_map_,
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
      primary_key,
      create_table.getProperties());
}

Status TSDBTableProvider::createDatabase(const String& database_name) {
  return Status(eRuntimeError, "permission denied");
}

Status TSDBTableProvider::alterTable(const csql::AlterTableNode& alter_table) {
  auto operations = alter_table.getOperations();
  Vector<TableService::AlterTableOperation> tbl_operations;
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

      TableService::AlterTableOperation operation;
      operation.optype = TableService::AlterTableOperationType::OP_ADD_COLUMN;
      operation.field_name = o.column_name;
      operation.field_type = type->second;
      operation.is_repeated = o.is_repeated;
      operation.is_optional = o.is_optional;
      tbl_operations.emplace_back(operation);

    } else {
      TableService::AlterTableOperation operation;
      operation.optype = TableService::AlterTableOperationType::OP_REMOVE_COLUMN;
      operation.field_name = o.column_name;
      tbl_operations.emplace_back(operation);
    }
  }

  return table_service_->alterTable(
      tsdb_namespace_,
      alter_table.getTableName(),
      tbl_operations);
}

Status TSDBTableProvider::insertRecord(
    const String& table_name,
    Vector<Pair<String, csql::SValue>> data) {

  auto schema = table_service_->tableSchema(tsdb_namespace_, table_name);
  if (schema.isEmpty()) {
    return Status(eRuntimeError, "table not found");
  }

  auto columns = schema.get()->columns();
  auto msg = new msg::DynamicMessage(schema.get());
  for (size_t i = 0; i < data.size(); ++i) {
    String column = data[i].first;
    if (column.empty()) {
      if (i >= columns.size()) {
        return Status(eRuntimeError, "more values than table columns"); //FIXME better msg
      }

      column = columns[i].first;
    }

    if (!msg->addField(column, data[i].second.getString())) {
      return Status(
          eRuntimeError,
          StringUtil::format("field not found: $0", column)); //FIXME better error msg
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

  auto pkey = table.config.config().primary_key();
  for (const auto& col : table.schema->columns()) {
    csql::ColumnInfo ci;
    ci.column_name = col.first;
    ci.type = col.second.typeName();
    ci.type_size = col.second.typeSize();
    ci.is_nullable = col.second.optional;
    ci.is_primary_key = std::find(
      pkey.begin(), pkey.end(), col.first) != pkey.end();

    ti.columns.emplace_back(ci);
  }

  return ti;
}

const String& TSDBTableProvider::getNamespace() const {
  return tsdb_namespace_;
}

RefPtr<csql::ValueExpressionNode> TSDBTableProvider::simplifyWhereExpression(
      RefPtr<Table> table,
      const String& keyrange_begin,
      const String& keyrange_end,
      RefPtr<csql::ValueExpressionNode> expr) const {
  auto pkey_col = table->getPartitionKey();
  auto pkeyspace = table->getKeyspaceType();

  Vector<csql::ScanConstraint> constraints;
  csql::QueryTreeUtil::findConstraints(expr, &constraints);

  for (const auto& c : constraints) {
    if (c.column_name != pkey_col) {
      continue;
    }

    switch (c.type) {

      case csql::ScanConstraintType::EQUAL_TO:
      case csql::ScanConstraintType::NOT_EQUAL_TO:
        continue;

      case csql::ScanConstraintType::LESS_THAN:
      case csql::ScanConstraintType::LESS_THAN_OR_EQUAL_TO:
        if (keyrange_end.size() > 0 &&
            comparePartitionKeys(
                pkeyspace,
                encodePartitionKey(pkeyspace, c.value.getString()),
                keyrange_end) >= 0) {
          break;
        } else {
          continue;
        }

      case csql::ScanConstraintType::GREATER_THAN:
      case csql::ScanConstraintType::GREATER_THAN_OR_EQUAL_TO:
        if (keyrange_begin.size() > 0 &&
            comparePartitionKeys(
                pkeyspace,
                encodePartitionKey(pkeyspace, c.value.getString()),
                keyrange_begin) <= 0) {
          break;
        } else {
          continue;
        }

    }

    expr = csql::QueryTreeUtil::removeConstraintFromPredicate(expr, c);
  }

  return expr;
}

void evqlVersionExpr(sql_txn* ctx, int argc, csql::SValue* argv, csql::SValue* out) {
  *out = csql::SValue::newString("EventQL " + eventql::kVersionString);
}

} // namespace csql
