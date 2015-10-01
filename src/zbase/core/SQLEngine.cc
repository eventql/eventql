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
#include <csql/qtree/RemoteAggregateParams.pb.h>

namespace zbase {

RefPtr<csql::QueryTreeNode> SQLEngine::rewriteQuery(
    csql::Runtime* runtime,
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    CSTableIndex* cstable_index,
    const String& tsdb_namespace,
    RefPtr<csql::QueryTreeNode> query) {
  insertPartitionSubqueries(
      runtime,
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
    csql::Runtime* runtime,
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
        runtime,
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
        runtime,
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
    csql::Runtime* runtime,
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

    auto copy = node->get()->deepCopy();
    auto copy_seq_scan = copy->child(0).asInstanceOf<csql::SequentialScanNode>();
    copy_seq_scan->setTableName(table_name);

    if (replication_scheme->hasLocalReplica(partition)) {
      shards.emplace_back(copy.get());
    } else {
      auto replicas = replication_scheme->replicaAddrsFor(partition);

      auto remote_aggr = mkRef(
          new csql::RemoteAggregateNode(
              copy.asInstanceOf<csql::GroupByNode>(),
              std::bind(
                  &SQLEngine::executeRemoteGroupBy,
                  partition_map,
                  replication_scheme,
                  cstable_index,
                  tsdb_namespace,
                  replicas,
                  std::placeholders::_1)));

      shards.emplace_back(remote_aggr.get());
    }
  }

  *node = RefPtr<csql::QueryTreeNode>(new csql::GroupByMergeNode(shards));
}

RefPtr<csql::ExecutionStrategy> SQLEngine::getExecutionStrategy(
    csql::Runtime* runtime,
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    CSTableIndex* cstable_index,
    const String& customer) {
  auto strategy = mkRef(new csql::DefaultExecutionStrategy());

  strategy->addTableProvider(
      tableProviderForNamespace(partition_map, cstable_index, customer));

  strategy->addQueryTreeRewriteRule(
      std::bind(
          &zbase::SQLEngine::rewriteQuery,
          runtime,
          partition_map,
          replication_scheme,
          cstable_index,
          customer,
          std::placeholders::_1));

  return strategy.get();
}

ScopedPtr<InputStream> SQLEngine::executeRemoteGroupBy(
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    CSTableIndex* cstable_index,
    const String& customer,
    const Vector<InetAddr>& hosts,
    const csql::RemoteAggregateParams& params) {
  Vector<String> errors;

  for (const auto& host : hosts) {
    try {
      return executeRemoteGroupByOnHost(
          partition_map,
          replication_scheme,
          cstable_index,
          customer,
          host,
          params);
    } catch (const StandardException& e) {
      logError(
          "zbase",
          e,
          "LogfileService::scanRemoteLogfilePartition failed");

      errors.emplace_back(e.what());
    }
  }

  RAISEF(
      kRuntimeError,
      "LogfileService::scanRemoteLogfilePartition failed: $0",
      StringUtil::join(errors, ", "));
}

ScopedPtr<InputStream> SQLEngine::executeRemoteGroupByOnHost(
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    CSTableIndex* cstable_index,
    const String& customer,
    const InetAddr& host,
    const csql::RemoteAggregateParams& params) {
  iputs("execute group by on $0 -> $1", host.hostAndPort(), params.DebugString());
  RAISE(kNotImplementedError);
}

}
