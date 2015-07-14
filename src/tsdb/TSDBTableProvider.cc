/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord/SHA1.h>
#include <tsdb/TSDBTableProvider.h>
#include <tsdb/TSDBNode.h>
#include <chartsql/CSTableScan.h>
#include <chartsql/runtime/EmptyTable.h>

using namespace fnord;

namespace tsdb {

TSDBTableProvider::TSDBTableProvider(
    const String& tsdb_namespace,
    TSDBNode* node) :
    tsdb_namespace_(tsdb_namespace),
    tsdb_node_(node) {}

Option<ScopedPtr<csql::TableExpression>> TSDBTableProvider::buildSequentialScan(
      RefPtr<csql::SequentialScanNode> node,
      csql::QueryBuilder* runtime) const {
  auto table_ref = TSDBTableRef::parse(node->tableName());

  if (table_ref.host.isEmpty() || table_ref.host.get() != "localhost") {
    RAISEF(
        kRuntimeError,
        "error while opening table '$0': host must be == localhost",
        node->tableName());
  }

  if (table_ref.partition_key.isEmpty()) {
    RAISEF(
        kRuntimeError,
        "error while opening table '$0': missing partition key",
        node->tableName());
  }

  auto partition = tsdb_node_->findPartition(
      tsdb_namespace_,
      table_ref.table_key,
      table_ref.partition_key.get());

  if (partition.isEmpty()) {
    return Option<ScopedPtr<csql::TableExpression>>(
        mkScoped(new csql::EmptyTable()));
  }

  auto cstable = partition.get()->cstable();
  if (cstable.isEmpty()) {
    return Option<ScopedPtr<csql::TableExpression>>(
        mkScoped(new csql::EmptyTable()));
  }

  return Option<ScopedPtr<csql::TableExpression>>(
      mkScoped(
          new csql::CSTableScan(
              node,
              std::move(cstable.get()),
              runtime)));
}


void TSDBTableProvider::listTables(
    Function<void (const csql::TableInfo& table)> fn) const {
  tsdb_node_->listTables([this, fn] (const TSDBTableInfo& table) {
    fn(tableInfoForTable(table));
  });
}

Option<csql::TableInfo> TSDBTableProvider::describe(
    const String& table_name) const {
  auto table = tsdb_node_->tableInfo(tsdb_namespace_, table_name);
  if (table.isEmpty()) {
    return None<csql::TableInfo>();
  } else {
    return Some(tableInfoForTable(table.get()));
  }
}

csql::TableInfo TSDBTableProvider::tableInfoForTable(
    const TSDBTableInfo& table) const {
  csql::TableInfo ti;
  ti.table_name = table.table_name;

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

} // namespace csql
