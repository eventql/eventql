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
#include <eventql/util/SHA1.h>
#include <eventql/server/sql/table_provider.h>
//#include <eventql/server/sql/table_scan.h>
#include <eventql/db/TSDBService.h>
#include <eventql/sql/CSTableScan.h>

#include "eventql/eventql.h"

namespace eventql {

TSDBTableProvider::TSDBTableProvider(
    const String& tsdb_namespace,
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    AnalyticsAuth* auth) :
    tsdb_namespace_(tsdb_namespace),
    partition_map_(partition_map),
    replication_scheme_(replication_scheme),
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
