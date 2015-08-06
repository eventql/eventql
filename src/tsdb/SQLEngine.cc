/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/SQLEngine.h>
#include <tsdb/TSDBService.h>
#include <tsdb/TimeWindowPartitioner.h>
#include <tsdb/FixedShardPartitioner.h>
#include <chartsql/defaults.h>

namespace tsdb {

RefPtr<csql::QueryTreeNode> SQLEngine::rewriteQuery(
    PartitionMap* partition_map,
    CSTableIndex* cstable_index,
    const String& tsdb_namespace,
    RefPtr<csql::QueryTreeNode> query) {
  replaceAllSequentialScansWithUnions(
      partition_map,
      cstable_index,
      tsdb_namespace,
      &query);
  return query;
}

RefPtr<csql::TableProvider> SQLEngine::tableProviderForNamespace(
    PartitionMap* partition_map,
    CSTableIndex* cstable_index,
    const String& tsdb_namespace) {
  return new TSDBTableProvider(tsdb_namespace, partition_map, cstable_index);
}

void SQLEngine::replaceAllSequentialScansWithUnions(
    PartitionMap* partition_map,
    CSTableIndex* cstable_index,
    const String& tsdb_namespace,
    RefPtr<csql::QueryTreeNode>* node) {
  if (dynamic_cast<csql::SequentialScanNode*>(node->get())) {
    replaceSequentialScanWithUnion(
        partition_map,
        cstable_index,
        tsdb_namespace,
        node);
    return;
  }

  auto ntables = node->get()->numChildren();
  for (int i = 0; i < ntables; ++i) {
    replaceAllSequentialScansWithUnions(
        partition_map,
        cstable_index,
        tsdb_namespace,
        node->get()->mutableChild(i));
  }
}

void SQLEngine::replaceSequentialScanWithUnion(
    PartitionMap* partition_map,
    CSTableIndex* cstable_index,
    const String& tsdb_namespace,
    RefPtr<csql::QueryTreeNode>* node) {
  auto seqscan = node->asInstanceOf<csql::SequentialScanNode>();

  auto table_ref = TSDBTableRef::parse(seqscan->tableName());
  if (!table_ref.partition_key.isEmpty()) {
    return;
  }

  auto table = partition_map->findTable(tsdb_namespace, table_ref.table_key);
  if (table.isEmpty()) {
    return;
  }

  Vector<SHA1Hash> partitions;
  switch (table.get()->partitioner()) {

    case TBL_PARTITION_TIMEWINDOW: {
      if (table_ref.timerange_begin.isEmpty() ||
          table_ref.timerange_limit.isEmpty()) {
        RAISEF(
            kRuntimeError,
            "invalid reference to timeseries table '$0' without timerange. " \
            "try appending .last30days to the table name",
            table_ref.table_key);
      }

      partitions = TimeWindowPartitioner::partitionKeysFor(
          table_ref.table_key,
          table_ref.timerange_begin.get(),
          table_ref.timerange_limit.get(),
          4 * kMicrosPerHour);

      break;
    }

    case TBL_PARTITION_FIXED: {
      partitions = FixedShardPartitioner::partitionKeysFor(
          table_ref.table_key,
          table.get()->numShards());

      break;
    }

  }

  Vector<RefPtr<csql::QueryTreeNode>> union_tables;
  for (const auto& partition : partitions) {
    auto table_name = StringUtil::format(
        "tsdb://localhost/$0/$1",
        URI::urlEncode(table_ref.table_key),
        partition.toString());

    auto copy = seqscan->deepCopyAs<csql::SequentialScanNode>();
    copy->setTableName(table_name);
    union_tables.emplace_back(copy.get());
  }

  *node = RefPtr<csql::QueryTreeNode>(new csql::UnionNode(union_tables));
}

}
