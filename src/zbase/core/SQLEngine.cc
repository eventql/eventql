/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <zbase/core/SQLEngine.h>
#include <zbase/core/TSDBService.h>
#include <zbase/core/TimeWindowPartitioner.h>
#include <zbase/core/FixedShardPartitioner.h>
#include <csql/defaults.h>
#include <csql/qtree/GroupByMergeNode.h>
#include <csql/qtree/RemoteAggregateNode.h>
#include <zbase/RemoteAggregateParams.pb.h>

namespace zbase {

RefPtr<csql::QueryTreeNode> SQLEngine::rewriteQuery(
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    CSTableIndex* cstable_index,
    const String& tsdb_namespace,
    RefPtr<csql::QueryTreeNode> query) {
  insertPartitionSubqueries(
      partition_map,
      replication_scheme,
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

void SQLEngine::insertPartitionSubqueries(
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    CSTableIndex* cstable_index,
    const String& tsdb_namespace,
    RefPtr<csql::QueryTreeNode>* node) {
  // rewrite group by -> sequential scan pairs to sharded group bys
  if (dynamic_cast<csql::GroupByNode*>(node->get()) &&
      node->get()->numChildren() == 1 &&
      (dynamic_cast<csql::SequentialScanNode*>(node->get()->child(0).get()))) {
    shardGroupBy(
        partition_map,
        replication_scheme,
        cstable_index,
        tsdb_namespace,
        node);
    return;
  }

  // replace remaining sequential scan nodes with unions over all partitions
  if (dynamic_cast<csql::SequentialScanNode*>(node->get())) {
    replaceSequentialScanWithUnion(
        partition_map,
        replication_scheme,
        cstable_index,
        tsdb_namespace,
        node);
    return;
  }

  auto ntables = node->get()->numChildren();
  for (int i = 0; i < ntables; ++i) {
    insertPartitionSubqueries(
        partition_map,
        replication_scheme,
        cstable_index,
        tsdb_namespace,
        node->get()->mutableChild(i));
  }
}

void SQLEngine::replaceSequentialScanWithUnion(
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
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

  auto partitioner = table.get()->partitioner();
  auto partitions = partitioner->partitionKeysFor(table_ref);

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

void SQLEngine::shardGroupBy(
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    CSTableIndex* cstable_index,
    const String& tsdb_namespace,
    RefPtr<csql::QueryTreeNode>* node) {
  auto seqscan = node->get()->child(0).asInstanceOf<csql::SequentialScanNode>();

  auto table_ref = TSDBTableRef::parse(seqscan->tableName());
  if (!table_ref.partition_key.isEmpty()) {
    return;
  }

  auto table = partition_map->findTable(tsdb_namespace, table_ref.table_key);
  if (table.isEmpty()) {
    return;
  }

  auto partitioner = table.get()->partitioner();
  auto partitions = partitioner->partitionKeysFor(table_ref);

  Vector<RefPtr<csql::QueryTreeNode>> shards;
  for (const auto& partition : partitions) {
    auto table_name = StringUtil::format(
        "tsdb://localhost/$0/$1",
        URI::urlEncode(table_ref.table_key),
        partition.toString());

    if (replication_scheme->hasLocalReplica(partition)) {
      auto copy = node->get()->deepCopy();
      auto copy_seq_scan = copy->child(0).asInstanceOf<csql::SequentialScanNode>();
      copy_seq_scan->setTableName(table_name);
      shards.emplace_back(copy.get());
    } else {
      iputs("scan remote.. $0", partition.toString());
      auto seq_scan = node->get()->child(0).asInstanceOf<csql::SequentialScanNode>();

      RemoteAggregateParams params;
      params.set_table_name(table_name);
      params.set_aggregation_strategy((uint64_t) seq_scan->aggregationStrategy());

      for (const auto& e :
            node->asInstanceOf<csql::GroupByNode>()->groupExpressions()) {
        *params.add_group_expression_list() = e->toSQL();
      }

      for (const auto& e :
            node->asInstanceOf<csql::GroupByNode>()->selectList()) {
        auto sl = params.add_aggregate_expression_list();
        sl->set_expression(e->expression()->toSQL());
        sl->set_alias(e->columnName());
      }

      for (const auto& e : seq_scan->selectList()) {
        auto sl = params.add_select_expression_list();
        sl->set_expression(e->expression()->toSQL());
        sl->set_alias(e->columnName());
      }

      auto where = seq_scan->whereExpression();
      if (!where.isEmpty()) {
        params.set_where_expression(where.get()->toSQL());
      }

      iputs("params: $0", params.DebugString());
      //auto remote_aggr = mkRef(new csql::RemoteAggregateNode());
      //shards.emplace_back(remote_aggr.get());
    }
  }

  *node = RefPtr<csql::QueryTreeNode>(new csql::GroupByMergeNode(shards));
}

}
