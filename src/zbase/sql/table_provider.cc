/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/SHA1.h>
#include <zbase/sql/table_provider.h>
#include <zbase/core/TSDBService.h>
#include <zbase/core/RemoteTSDBScan.h>
#include <csql/CSTableScan.h>

using namespace stx;

namespace zbase {

TSDBTableProvider::TSDBTableProvider(
    const String& tsdb_namespace,
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    AnalyticsAuth* auth) :
    tsdb_namespace_(tsdb_namespace),
    partition_map_(partition_map),
    replication_scheme_(replication_scheme),
    auth_(auth) {}

csql::TaskIDList TSDBTableProvider::buildSequentialScan(
    csql::Transaction* txn,
    RefPtr<csql::SequentialScanNode> node,
    csql::TaskDAG* tasks) const {
  auto table_ref = TSDBTableRef::parse(node->tableName());
  if (partition_map_->findTable(tsdb_namespace_, table_ref.table_key).isEmpty()) {
    return csql::TaskIDList{};
  }

  if (table_ref.partition_key.isEmpty()) {
    RAISEF(
        kRuntimeError,
        "error while opening table '$0': missing partition key",
        node->tableName());
  }

  if (table_ref.host.isEmpty() || table_ref.host.get() != "localhost") {
    return buildRemoteSequentialScan(txn, node, table_ref, tasks);
  } else {
    return buildLocalSequentialScan(txn, node, table_ref, tasks);
  }
}

csql::TaskIDList TSDBTableProvider::buildLocalSequentialScan(
    csql::Transaction* txn,
    RefPtr<csql::SequentialScanNode> node,
    const TSDBTableRef& table_ref,
    csql::TaskDAG* tasks) const {

  auto partition = partition_map_->findPartition(
      tsdb_namespace_,
      table_ref.table_key,
      table_ref.partition_key.get());

  if (partition.isEmpty()) {
    return csql::TaskIDList{};
  } else {
    auto reader = partition.get()->getReader();
    return reader->buildSQLScan(txn, node, tasks);
  }
}

csql::TaskIDList TSDBTableProvider::buildRemoteSequentialScan(
    csql::Transaction* txn,
    RefPtr<csql::SequentialScanNode> node,
    const TSDBTableRef& table_ref,
    csql::TaskDAG* tasks) const {
  auto task_factory = [this, node, table_ref] (
      csql::Transaction* txn,
      csql::RowSinkFn output) -> RefPtr<csql::Task> {
      return new RemoteTSDBScan(
          node,
          tsdb_namespace_,
          table_ref,
          replication_scheme_,
          auth_);
  };

  auto task = new csql::TaskDAGNode(
      new csql::SimpleTableExpressionFactory(task_factory));

  csql::TaskIDList output;
  output.emplace_back(tasks->addTask(task));
  return output;
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
