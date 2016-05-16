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
#include <eventql/sql/qtree/QueryTreeUtil.h>

#include "eventql/eventql.h"

namespace eventql {

Scheduler::Scheduler(PartitionMap* pmap) : pmap_(pmap) {}

ScopedPtr<csql::TableExpression> Scheduler::buildLimit(
    csql::Transaction* ctx,
    RefPtr<csql::LimitNode> node) {
  return mkScoped(
      new csql::LimitExpression(
          node->limit(),
          node->offset(),
          buildExpression(ctx, node->inputTable())));
}

ScopedPtr<csql::TableExpression> Scheduler::buildSelectExpression(
    csql::Transaction* ctx,
    RefPtr<csql::SelectExpressionNode> node) {
  Vector<csql::ValueExpression> select_expressions;
  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        ctx->getCompiler()->buildValueExpression(ctx, slnode->expression()));
  }

  return mkScoped(new csql::SelectExpression(
      ctx,
      std::move(select_expressions)));
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

  return mkScoped(new csql::SubqueryExpression(
      txn,
      std::move(select_expressions),
      std::move(where_expr),
      buildExpression(txn, node->subquery())));
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

  return mkScoped(
      new csql::OrderByExpression(
          txn,
          std::move(sort_exprs),
          buildExpression(txn, node->inputTable())));
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

  return mkScoped(
      new csql::GroupByExpression(
          txn,
          std::move(select_expressions),
          std::move(group_expressions),
          buildExpression(txn, node->inputTable())));
}

ScopedPtr<csql::TableExpression> Scheduler::buildPipelineGroupByExpression(
      csql::Transaction* txn,
      RefPtr<csql::GroupByNode> node) {
  auto shards = pipelineExpression(txn, node.get());
  RAISE(kNotYetImplementedError);
}

Vector<RefPtr<csql::QueryTreeNode>> Scheduler::pipelineExpression(
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
      static_cast<TransactionInfo*>(user_data)->getNamespace(),
      table_ref.table_key);
  if (table.isEmpty()) {
    RAISE(kIllegalStateError, "can't pipeline query tree");
  }

  auto partitioner = table.get()->partitioner();
  auto partitions = partitioner->listPartitions(seqscan->constraints());

  Vector<RefPtr<csql::QueryTreeNode>> shards;
  for (const auto& partition : partitions) {
    auto table_name = StringUtil::format(
        "tsdb://localhost/$0/$1",
        URI::urlEncode(table_ref.table_key),
        partition.toString());

    auto qtree_copy = qtree->deepCopy();
    auto shard = csql::QueryTreeUtil::findNode<csql::SequentialScanNode>(
        qtree_copy.get());
    shard->setTableName(table_name);
    shards.emplace_back(shard);
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

  return mkScoped(
      new csql::NestedLoopJoin(
          ctx,
          node->joinType(),
          node->inputColumnMap(),
          std::move(select_expressions),
          std::move(join_cond_expr),
          std::move(where_expr),
          buildExpression(ctx, node->baseTable()),
          buildExpression(ctx, node->joinedTable())));
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
    size_t stmt_idx) {
  return mkScoped(
      new csql::TableExpressionResultCursor(
          buildExpression(
              query_plan->getTransaction(),
              query_plan->getStatement(stmt_idx))));
};

bool Scheduler::isPipelineable(const csql::QueryTreeNode& qtree) {
  return true;
}



} // namespace eventql

