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
#include <eventql/server/sql/scheduler.h>
#include <eventql/server/sql/table_provider.h>
#include <eventql/server/sql/pipelined_expression.h>
#include <eventql/sql/qtree/QueryTreeUtil.h>
#include <eventql/db/metadata_client.h>

#include "eventql/eventql.h"

namespace eventql {

Scheduler::Scheduler(
    PartitionMap* pmap,
    ConfigDirectory* cdir,
    InternalAuth* auth,
    ReplicationScheme* repl_scheme) :
    pmap_(pmap),
    cdir_(cdir),
    auth_(auth),
    repl_scheme_(repl_scheme),
    running_cnt_(0) {}

ScopedPtr<csql::TableExpression> Scheduler::buildTableExpression(
    csql::Transaction* txn,
    csql::ExecutionContext* execution_context,
    RefPtr<csql::TableExpressionNode> node) {
  rewriteTableTimeSuffix(node.get());

  return DefaultScheduler::buildTableExpression(
      txn,
      execution_context,
      node.asInstanceOf<csql::TableExpressionNode>());
}

ScopedPtr<csql::TableExpression> Scheduler::buildGroupByExpression(
    csql::Transaction* txn,
    csql::ExecutionContext* execution_context,
    RefPtr<csql::GroupByNode> node) {
  if (node->isPartialAggregation()) {
    return buildPartialGroupByExpression(
        txn,
        execution_context,
        node);
  }

  if (isPipelineable(*node->inputTable())) {
    return buildPipelineGroupByExpression(
        txn,
        execution_context,
        node);
  } else {
    return csql::DefaultScheduler::buildGroupByExpression(
        txn,
        execution_context,
        node);
  }
}

ScopedPtr<csql::TableExpression> Scheduler::buildPartialGroupByExpression(
    csql::Transaction* txn,
    csql::ExecutionContext* execution_context,
    RefPtr<csql::GroupByNode> node) {
  Vector<csql::ValueExpression> select_expressions;
  Vector<csql::ValueExpression> group_expressions;

  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        txn->getCompiler()->buildValueExpression(
            txn,
            slnode->expression()));
  }

  for (const auto& e : node->groupExpressions()) {
    group_expressions.emplace_back(
        txn->getCompiler()->buildValueExpression(txn, e));
  }

  return mkScoped(
      new csql::PartialGroupByExpression(
          txn,
          std::move(select_expressions),
          std::move(group_expressions),
          buildTableExpression(
              txn,
              execution_context,
              node->inputTable().asInstanceOf<csql::TableExpressionNode>())));
}

ScopedPtr<csql::TableExpression> Scheduler::buildPipelineGroupByExpression(
    csql::Transaction* txn,
    csql::ExecutionContext* execution_context,
    RefPtr<csql::GroupByNode> node) {
  auto remote_aggregate = mkScoped(
      new PipelinedExpression(
          txn,
          execution_context,
          static_cast<Session*>(txn->getUserData())->getEffectiveNamespace(),
          auth_,
          kMaxConcurrency));

  auto shards = pipelineExpression(txn, node.get());
  for (size_t i = 0; i < shards.size(); ++i) {
    auto group_by_copy = mkRef(
        new csql::GroupByNode(
            node->selectList(),
            node->groupExpressions(),
            shards[i].qtree));

    group_by_copy->setIsPartialAggreagtion(true);
    if (shards[i].is_local) {
      auto partial = 
          buildPartialGroupByExpression(txn, execution_context, group_by_copy);
      remote_aggregate->addLocalQuery(std::move(partial));
    } else {
      remote_aggregate->addRemoteQuery(group_by_copy.get(), shards[i].hosts);
    }
  }

  Vector<csql::ValueExpression> select_expressions;
  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        txn->getCompiler()->buildValueExpression(
            txn,
            slnode->expression()));
  }

  return mkScoped(
      new csql::GroupByMergeExpression(
          txn,
          execution_context,
          std::move(select_expressions),
          std::move(remote_aggregate)));
}

Vector<Scheduler::PipelinedQueryTree> Scheduler::pipelineExpression(
    csql::Transaction* txn,
    RefPtr<csql::QueryTreeNode> qtree) {
  auto seqscan = csql::QueryTreeUtil::findNode<csql::SequentialScanNode>(
      qtree.get());
  if (!seqscan) {
    RAISE(kIllegalStateError, "can't pipeline query tree");
  }

  auto table_ref = TSDBTableRef::parse(seqscan->tableName());
  if (!table_ref.partition_key.isEmpty()) {
    RAISE(kIllegalStateError, "can't pipeline query tree");
  }

  auto user_data = txn->getUserData();
  if (user_data == nullptr) {
    RAISE(kRuntimeError, "no user data");
  }

  auto table = pmap_->findTable(
      static_cast<Session*>(txn->getUserData())->getEffectiveNamespace(),
      table_ref.table_key);
  if (table.isEmpty()) {
    RAISE(kIllegalStateError, "can't pipeline query tree");
  }

  auto db_namespace =
      static_cast<Session*>(txn->getUserData())->getEffectiveNamespace();

  String local_server_id;
  if (cdir_->hasServerID()) {
    local_server_id = cdir_->getServerID();
  }

  HashMap<SHA1Hash, Vector<ReplicaRef>> partitions;
  Set<SHA1Hash> local_partitions;
  if (table.get()->partitionerType() == TBL_PARTITION_TIMEWINDOW &&
      table.get()->storage() == TBL_STORAGE_COLSM) {
    auto keyrange = TSDBTableProvider::findKeyRange(
        table.get()->config().config().partition_key(),
        seqscan->constraints());

    MetadataClient metadata_client(cdir_);
    PartitionListResponse partition_list;
    auto rc = metadata_client.listPartitions(
        db_namespace,
        table_ref.table_key,
        keyrange,
        &partition_list);

    if (!rc.isSuccess()) {
      RAISEF(kRuntimeError, "metadata lookup failure: $0", rc.message());
    }

    for (const auto& p : partition_list.partitions()) {
      Vector<ReplicaRef> replicas;
      SHA1Hash pid(p.partition_id().data(), p.partition_id().size());

      for (const auto& s : p.servers()) {
        if (s == local_server_id) {
          local_partitions.emplace(pid);
        }

        auto server_cfg = cdir_->getServerConfig(s);
        if (server_cfg.server_status() != SERVER_UP) {
          continue;
        }

        ReplicaRef rref(SHA1::compute(s), server_cfg.server_addr());
        rref.name = s;
        replicas.emplace_back(rref);
      }

      partitions.emplace(pid, replicas);
    }
  } else {
    auto partitioner = table.get()->partitioner();
    for (const auto& p : partitioner->listPartitions(seqscan->constraints())) {
      auto replicas = repl_scheme_->replicasFor(p);
      if (repl_scheme_->hasLocalReplica(p)) {
        local_partitions.emplace(p);
      }

      partitions.emplace(p, Vector<ReplicaRef>(replicas.begin(), replicas.end()));
    }
  }

  Vector<PipelinedQueryTree> shards;
  for (const auto& p : partitions) {
    auto table_name = StringUtil::format(
        "tsdb://localhost/$0/$1",
        URI::urlEncode(table_ref.table_key),
        p.first.toString());

    auto qtree_copy = qtree->deepCopy();
    auto shard = csql::QueryTreeUtil::findNode<csql::SequentialScanNode>(
        qtree_copy.get());
    shard->setTableName(table_name);

    PipelinedQueryTree pipelined {
      .is_local = local_partitions.count(p.first) > 0,
      .qtree = shard,
      .hosts = p.second
    };

    shards.emplace_back(pipelined);
  }

  return shards;
}

bool Scheduler::isPipelineable(const csql::QueryTreeNode& qtree) {
  if (dynamic_cast<const csql::SequentialScanNode*>(&qtree)) {
    return true;
  }

  if (dynamic_cast<const csql::SelectExpressionNode*>(&qtree)) {
    return true;
  }

  if (dynamic_cast<const csql::SubqueryNode*>(&qtree)) {
    return isPipelineable(
        *dynamic_cast<const csql::SubqueryNode&>(qtree).subquery());
  }

  return false;
}

void Scheduler::rewriteTableTimeSuffix(RefPtr<csql::QueryTreeNode> node) {
  auto seqscan = dynamic_cast<csql::SequentialScanNode*>(node.get());
  if (seqscan) {
    auto table_ref = TSDBTableRef::parse(seqscan->tableName());
    if (!table_ref.timerange_begin.isEmpty() &&
        !table_ref.timerange_limit.isEmpty()) {
      seqscan->setTableName(table_ref.table_key);

      auto pred = mkRef(
          new csql::CallExpressionNode(
              "logical_and",
              Vector<RefPtr<csql::ValueExpressionNode>>{
                new csql::CallExpressionNode(
                    "gte",
                    Vector<RefPtr<csql::ValueExpressionNode>>{
                      new csql::ColumnReferenceNode("time"),
                      new csql::LiteralExpressionNode(
                          csql::SValue(csql::SValue::IntegerType(
                              table_ref.timerange_begin.get().unixMicros())))
                    }),
                new csql::CallExpressionNode(
                    "lte",
                    Vector<RefPtr<csql::ValueExpressionNode>>{
                      new csql::ColumnReferenceNode("time"),
                      new csql::LiteralExpressionNode(
                          csql::SValue(csql::SValue::IntegerType(
                              table_ref.timerange_limit.get().unixMicros())))
                    })
              }));

      auto where_expr = seqscan->whereExpression();
      if (!where_expr.isEmpty()) {
        pred = mkRef(
            new csql::CallExpressionNode(
                "logical_and",
                Vector<RefPtr<csql::ValueExpressionNode>>{
                  where_expr.get(),
                  pred.asInstanceOf<csql::ValueExpressionNode>()
                }));
      }

      seqscan->setWhereExpression(
          pred.asInstanceOf<csql::ValueExpressionNode>());
    }
  }

  auto ntables = node->numChildren();
  for (int i = 0; i < ntables; ++i) {
    rewriteTableTimeSuffix(node->child(i));
  }
}


} // namespace eventql

