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
#include <eventql/server/sql/transaction_info.h>
#include <eventql/server/sql/pipelined_expression.h>
#include <eventql/sql/qtree/QueryTreeUtil.h>

#include "eventql/eventql.h"

namespace eventql {

Scheduler::Scheduler(
    PartitionMap* pmap,
    AnalyticsAuth* auth,
    ReplicationScheme* repl_scheme) :
    pmap_(pmap),
    auth_(auth),
    repl_scheme_(repl_scheme),
    running_cnt_(0) {}

ScopedPtr<csql::TableExpression> Scheduler::buildLimit(
    csql::Transaction* ctx,
    RefPtr<csql::LimitNode> node) {
  auto expr = new csql::LimitExpression(
          node->limit(),
          node->offset(),
          buildExpression(ctx, node->inputTable()));
  return mkScoped(expr);
}

ScopedPtr<csql::TableExpression> Scheduler::buildSelectExpression(
    csql::Transaction* ctx,
    RefPtr<csql::SelectExpressionNode> node) {
  Vector<csql::ValueExpression> select_expressions;
  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        ctx->getCompiler()->buildValueExpression(ctx, slnode->expression()));
  }

  auto expr = new csql::SelectExpression(
      ctx,
      std::move(select_expressions));
  return mkScoped(expr);
};

ScopedPtr<csql::TableExpression> Scheduler::buildSubquery(
    csql::Transaction* txn,
    RefPtr<csql::SubqueryNode> node) {
  Vector<csql::ValueExpression> select_expressions;
  Option<csql::ValueExpression> where_expr;

  if (!node->whereExpression().isEmpty()) {
    where_expr = std::move(Option<csql::ValueExpression>(
        txn->getCompiler()->buildValueExpression(txn, node->whereExpression().get())));
  }

  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        txn->getCompiler()->buildValueExpression(txn, slnode->expression()));
  }

  auto expr = new csql::SubqueryExpression(
      txn,
      std::move(select_expressions),
      std::move(where_expr),
      buildExpression(txn, node->subquery()));
  return mkScoped(expr);
}

ScopedPtr<csql::TableExpression> Scheduler::buildOrderByExpression(
    csql::Transaction* txn,
    RefPtr<csql::OrderByNode> node) {
  Vector<csql::OrderByExpression::SortExpr> sort_exprs;
  for (const auto& ss : node->sortSpecs()) {
    csql::OrderByExpression::SortExpr se;
    se.descending = ss.descending;
    se.expr = txn->getCompiler()->buildValueExpression(txn, ss.expr);
    sort_exprs.emplace_back(std::move(se));
  }

  auto expr = new csql::OrderByExpression(
          txn,
          std::move(sort_exprs),
          buildExpression(txn, node->inputTable()));
  return mkScoped(expr);
}

ScopedPtr<csql::TableExpression> Scheduler::buildSequentialScan(
    csql::Transaction* txn,
    RefPtr<csql::SequentialScanNode> node) {
  const auto& table_name = node->tableName();
  auto table_provider = txn->getTableProvider();

  auto seqscan = table_provider->buildSequentialScan(txn, node);
  if (seqscan.isEmpty()) {
    RAISEF(kRuntimeError, "table not found: $0", table_name);
  }

  return std::move(seqscan.get());
}

ScopedPtr<csql::TableExpression> Scheduler::buildGroupByExpression(
    csql::Transaction* txn,
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

  auto expr = new csql::GroupByExpression(
          txn,
          std::move(select_expressions),
          std::move(group_expressions),
          buildExpression(txn, node->inputTable()));
  return mkScoped(expr);
}

ScopedPtr<csql::TableExpression> Scheduler::buildPartialGroupByExpression(
    csql::Transaction* txn,
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

  auto expr = new csql::PartialGroupByExpression(
          txn,
          std::move(select_expressions),
          std::move(group_expressions),
          buildExpression(txn, node->inputTable()));
  return mkScoped(expr);
}

ScopedPtr<csql::TableExpression> Scheduler::buildPipelineGroupByExpression(
      csql::Transaction* txn,
      RefPtr<csql::GroupByNode> node) {
  auto remote_aggregate = mkScoped(
      new PipelinedExpression(
          txn,
          TransactionInfo::get(txn)->getNamespace(),
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
          buildPartialGroupByExpression(txn, group_by_copy);
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

  auto expr = new csql::GroupByMergeExpression(
          txn,
          std::move(select_expressions),
          std::move(remote_aggregate));
  return mkScoped(expr);
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
      TransactionInfo::get(txn)->getNamespace(),
      table_ref.table_key);
  if (table.isEmpty()) {
    RAISE(kIllegalStateError, "can't pipeline query tree");
  }

  auto partitioner = table.get()->partitioner();
  auto partitions = partitioner->listPartitions(seqscan->constraints());

  Vector<PipelinedQueryTree> shards;
  for (const auto& partition : partitions) {
    auto table_name = StringUtil::format(
        "tsdb://localhost/$0/$1",
        URI::urlEncode(table_ref.table_key),
        partition.toString());

    auto qtree_copy = qtree->deepCopy();
    auto shard = csql::QueryTreeUtil::findNode<csql::SequentialScanNode>(
        qtree_copy.get());
    shard->setTableName(table_name);

    auto shard_hosts = repl_scheme_->replicasFor(partition);

    shards.emplace_back(PipelinedQueryTree {
      .is_local = repl_scheme_->hasLocalReplica(partition),
      .qtree = shard,
      .hosts = shard_hosts
    });
  }

  return shards;
}


ScopedPtr<csql::TableExpression> Scheduler::buildJoinExpression(
    csql::Transaction* ctx,
    RefPtr<csql::JoinNode> node) {
  Vector<String> column_names;
  Vector<csql::ValueExpression> select_expressions;

  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        ctx->getCompiler()->buildValueExpression(ctx, slnode->expression()));
  }

  Option<csql::ValueExpression> where_expr;
  if (!node->whereExpression().isEmpty()) {
    where_expr = std::move(Option<csql::ValueExpression>(
        ctx->getCompiler()->buildValueExpression(ctx, node->whereExpression().get())));
  }

  Option<csql::ValueExpression> join_cond_expr;
  if (!node->joinCondition().isEmpty()) {
    join_cond_expr = std::move(Option<csql::ValueExpression>(
        ctx->getCompiler()->buildValueExpression(ctx, node->joinCondition().get())));
  }

  auto expr = new csql::NestedLoopJoin(
          ctx,
          node->joinType(),
          node->inputColumnMap(),
          std::move(select_expressions),
          std::move(join_cond_expr),
          std::move(where_expr),
          buildExpression(ctx, node->baseTable()),
          buildExpression(ctx, node->joinedTable()));
  return mkScoped(expr);
}

ScopedPtr<csql::TableExpression> Scheduler::buildExpression(
    csql::Transaction* ctx,
    RefPtr<csql::QueryTreeNode> node) {

  if (dynamic_cast<csql::LimitNode*>(node.get())) {
    return buildLimit(ctx, node.asInstanceOf<csql::LimitNode>());
  }

  if (dynamic_cast<csql::SelectExpressionNode*>(node.get())) {
    return buildSelectExpression(
        ctx,
        node.asInstanceOf<csql::SelectExpressionNode>());
  }

  if (dynamic_cast<csql::SubqueryNode*>(node.get())) {
    return buildSubquery(
        ctx,
        node.asInstanceOf<csql::SubqueryNode>());
  }

  if (dynamic_cast<csql::OrderByNode*>(node.get())) {
    return buildOrderByExpression(ctx, node.asInstanceOf<csql::OrderByNode>());
  }

  if (dynamic_cast<csql::SequentialScanNode*>(node.get())) {
    return buildSequentialScan(
        ctx,
        node.asInstanceOf<csql::SequentialScanNode>());
  }

  if (dynamic_cast<csql::GroupByNode*>(node.get())) {
    auto group_node = node.asInstanceOf<csql::GroupByNode>();
    if (group_node->isPartialAggregation()) {
      return buildPartialGroupByExpression(
          ctx,
          group_node);
    }

    if (isPipelineable(*group_node->inputTable())) {
      return buildPipelineGroupByExpression(
          ctx,
          group_node);
    } else {
      return buildGroupByExpression(
          ctx,
          group_node);
    }
  }

  if (dynamic_cast<csql::ShowTablesNode*>(node.get())) {
    return mkScoped(new csql::ShowTablesExpression(ctx));
  }

  if (dynamic_cast<csql::DescribeTableNode*>(node.get())) {
    return mkScoped(new csql::DescribeTableStatement(
        ctx,
        node.asInstanceOf<csql::DescribeTableNode>()->tableName()));
  }

  if (dynamic_cast<csql::JoinNode*>(node.get())) {
    return buildJoinExpression(
        ctx,
        node.asInstanceOf<csql::JoinNode>());
  }

  RAISEF(
      kRuntimeError,
      "cannot figure out how to execute that query, sorry. -- $0",
      node->toString());
};

ScopedPtr<csql::ResultCursor> Scheduler::execute(
    csql::QueryPlan* query_plan,
    csql::ExecutionContext* execution_context,
    size_t stmt_idx) {
  auto qtree = query_plan->getStatement(stmt_idx);
  rewriteTableTimeSuffix(qtree);

  return mkScoped(
      new csql::TableExpressionResultCursor(
          buildExpression(
              query_plan->getTransaction(),
              qtree)));
};

bool Scheduler::isPipelineable(const csql::QueryTreeNode& qtree) {
  return true;
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

