/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/http/httpclient.h>
#include <stx/protobuf/msg.h>
#include <zbase/core/SQLEngine.h>
#include <zbase/core/TSDBService.h>
#include <zbase/core/TimeWindowPartitioner.h>
#include <zbase/core/FixedShardPartitioner.h>
#include <csql/defaults.h>
#include <csql/qtree/GroupByMergeNode.h>
#include <csql/qtree/RemoteAggregateNode.h>
#include <csql/qtree/RemoteAggregateParams.pb.h>
#include <zbase/z1stats.h>

namespace zbase {

RefPtr<csql::QueryTreeNode> SQLEngine::rewriteQuery(
    csql::Runtime* runtime,
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    CompactionWorker* cstable_index,
    AnalyticsAuth* auth,
    const String& tsdb_namespace,
    RefPtr<csql::QueryTreeNode> query) {
  insertPartitionSubqueries(
      runtime,
      partition_map,
      replication_scheme,
      cstable_index,
      auth,
      tsdb_namespace,
      &query);
  return query;
}

RefPtr<csql::TableProvider> SQLEngine::tableProviderForNamespace(
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    CompactionWorker* cstable_index,
    AnalyticsAuth* auth,
    const String& tsdb_namespace) {
  return new TSDBTableProvider(
      tsdb_namespace,
      partition_map,
      replication_scheme,
      cstable_index,
      auth);
}

void SQLEngine::insertPartitionSubqueries(
    csql::Runtime* runtime,
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    CompactionWorker* cstable_index,
    AnalyticsAuth* auth,
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
        auth,
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
        auth,
        tsdb_namespace,
        node->get()->mutableChild(i));
  }
}

void SQLEngine::replaceSequentialScanWithUnion(
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    CompactionWorker* cstable_index,
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
  auto partitions = partitioner->listPartitions(seqscan->constraints());

  Vector<RefPtr<csql::QueryTreeNode>> union_tables;
  for (const auto& partition : partitions) {
    // String host = replication_scheme->hasLocalReplica(partition)
    //     ? "localhost"
    //     : "remote";

    auto table_name = StringUtil::format(
        "tsdb://remote/$0/$1",
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
    CompactionWorker* cstable_index,
    AnalyticsAuth* auth,
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
  auto partitions = partitioner->listPartitions(seqscan->constraints());

  Vector<RefPtr<csql::QueryTreeNode>> shards;
  for (const auto& partition : partitions) {
    auto table_name = StringUtil::format(
        "tsdb://localhost/$0/$1",
        URI::urlEncode(table_ref.table_key),
        partition.toString());

    auto copy = node->get()->deepCopy();
    auto copy_seq_scan = copy->child(0).asInstanceOf<csql::SequentialScanNode>();
    copy_seq_scan->setTableName(table_name);

    auto replicas = replication_scheme->replicasFor(partition);

    auto remote_aggr = mkRef(
        new csql::RemoteAggregateNode(
            copy.asInstanceOf<csql::GroupByNode>(),
            std::bind(
                &SQLEngine::executeParallelGroupBy,
                partition_map,
                replication_scheme,
                cstable_index,
                auth,
                tsdb_namespace,
                replicas,
                std::placeholders::_1)));

    shards.emplace_back(remote_aggr.get());
  }

  *node = RefPtr<csql::QueryTreeNode>(new csql::GroupByMergeNode(shards));
}

RefPtr<csql::ExecutionStrategy> SQLEngine::getExecutionStrategy(
    csql::Runtime* runtime,
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    CompactionWorker* cstable_index,
    AnalyticsAuth* auth,
    const String& customer) {
  auto strategy = mkRef(new csql::DefaultExecutionStrategy());

  strategy->addTableProvider(
      tableProviderForNamespace(
          partition_map,
          replication_scheme,
          cstable_index,
          auth,
          customer));

  strategy->addQueryTreeRewriteRule(
      std::bind(
          &zbase::SQLEngine::rewriteQuery,
          runtime,
          partition_map,
          replication_scheme,
          cstable_index,
          auth,
          customer,
          std::placeholders::_1));

  return strategy.get();
}

ScopedPtr<InputStream> SQLEngine::executeParallelGroupBy(
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    CompactionWorker* cstable_index,
    AnalyticsAuth* auth,
    const String& customer,
    const Vector<ReplicaRef>& hosts,
    const csql::RemoteAggregateParams& params) {
  Vector<String> errors;

  for (const auto& replica : hosts) {
    try {
      return executeRemoteGroupBy(
          partition_map,
          replication_scheme,
          cstable_index,
          auth,
          customer,
          replica.addr,
          params);
    } catch (const StandardException& e) {
      logError(
          "zbase",
          e,
          "SQLEngine::executeRemoteGroupBy failed");

      errors.emplace_back(e.what());
    }
  }

  RAISE(
      kRuntimeError,
      StringUtil::join(errors, ", "));
}

ScopedPtr<InputStream> SQLEngine::executeRemoteGroupBy(
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    CompactionWorker* cstable_index,
    AnalyticsAuth* auth,
    const String& customer,
    const InetAddr& host,
    const csql::RemoteAggregateParams& params) {
  logDebug(
      "zbase",
      "Executing remote aggreate on $0@$1",
      params.table_name(),
      host.hostAndPort());

  auto url = StringUtil::format(
      "http://$0/api/v1/sql/aggregate_partition",
      host.ipAndPort());

  AnalyticsPrivileges privileges;
  privileges.set_allow_private_api_read_access(true);
  auto api_token = auth->getPrivateAPIToken(customer, privileges);

  http::HTTPMessage::HeaderList auth_headers;
  auth_headers.emplace_back(
      "Authorization",
      StringUtil::format("Token $0", api_token));

  http::HTTPClient http_client(&z1stats()->http_client_stats);
  auto req_body = msg::encode(params);
  auto req = http::HTTPRequest::mkPost(url, *req_body, auth_headers);
  auto res = http_client.executeRequest(req);

  if (res.statusCode() != 200) {
    RAISE(kRuntimeError, res.body().toString());
  }

  return mkScoped(new StringInputStream(res.body().toString()));
}

}
