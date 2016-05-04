/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/TableExpressionBuilder.h>
//#include <csql/runtime/Union.h>
//#include <csql/runtime/SelectExpression.h>
//#include <csql/runtime/limitclause.h>
//#include <csql/runtime/ShowTablesStatement.h>
//#include <csql/runtime/DescribeTableStatement.h>
//#include <csql/runtime/SubqueryExpression.h>
//#include <csql/runtime/NestedLoopJoin.h>

using namespace stx;

namespace csql {

ScopedPtr<Task> TableExpressionBuilder::build(
    Transaction* ctx,
    RefPtr<QueryTreeNode> node,
    QueryBuilder* runtime,
    TableProvider* tables) {

  //if (dynamic_cast<LimitNode*>(node.get())) {
  //  return buildLimit(ctx, node.asInstanceOf<LimitNode>(), runtime, tables);
  //}

  //if (dynamic_cast<GroupByNode*>(node.get())) {
  //  return buildGroupBy(ctx, node.asInstanceOf<GroupByNode>(), runtime, tables);
  //}

  //if (dynamic_cast<GroupByMergeNode*>(node.get())) {
  //  return buildGroupMerge(
  //      ctx,
  //      node.asInstanceOf<GroupByMergeNode>(),
  //      runtime,
  //      tables);
  //}

  //if (dynamic_cast<UnionNode*>(node.get())) {
  //  return buildUnion(ctx, node.asInstanceOf<UnionNode>(), runtime, tables);
  //}

  //if (dynamic_cast<JoinNode*>(node.get())) {
  //  return buildJoin(
  //      ctx,
  //      node.asInstanceOf<JoinNode>(),
  //      runtime,
  //      tables);
  //}

  //if (dynamic_cast<SequentialScanNode*>(node.get())) {
  //  return buildSequentialScan(
  //      ctx,
  //      node.asInstanceOf<SequentialScanNode>(),
  //      runtime,
  //      tables);
  //}

  //if (dynamic_cast<SubqueryNode*>(node.get())) {
  //  return buildSubquery(
  //      ctx,
  //      node.asInstanceOf<SubqueryNode>(),
  //      runtime,
  //      tables);
  //}

  //if (dynamic_cast<SelectExpressionNode*>(node.get())) {
  //  return buildSelectExpression(
  //      ctx,
  //      node.asInstanceOf<SelectExpressionNode>(),
  //      runtime,
  //      tables);
  //}

  //if (dynamic_cast<ShowTablesNode*>(node.get())) {
  //  return mkScoped<Task>(new ShowTablesStatement(tables));
  //}

  //if (dynamic_cast<DescribeTableNode*>(node.get())) {
  //  return buildDescribeTableStatment(
  //      ctx,
  //      node.asInstanceOf<DescribeTableNode>(),
  //      runtime,
  //      tables);
  //}

  ////if (dynamic_cast<RemoteAggregateNode*>(node.get())) {
  ////  return buildRemoteAggregate(
  ////      ctx,
  ////      node.asInstanceOf<RemoteAggregateNode>(),
  ////      runtime);
  ////}

  RAISE(
      kRuntimeError,
      "cannot figure out how to build a table expression for this QTree node");
}

//ScopedPtr<Task> TableExpressionBuilder::buildGroupBy(
//    Transaction* ctx,
//    RefPtr<GroupByNode> node,
//    QueryBuilder* runtime,
//    TableProvider* tables) {
//  Vector<String> column_names;
//  Vector<ValueExpression> select_expressions;
//  Vector<ValueExpression> group_expressions;
//
//  for (const auto& slnode : node->selectList()) {
//    select_expressions.emplace_back(
//        runtime->buildValueExpression(ctx, slnode->expression()));
//  }
//
//  for (const auto& e : node->groupExpressions()) {
//    group_expressions.emplace_back(runtime->buildValueExpression(ctx, e));
//  }
//
//  auto next = build(ctx, node->inputTable(), runtime, tables);
//
//  return mkScoped(
//      new GroupBy(
//          ctx,
//          std::move(next),
//          node->outputColumns(),
//          std::move(select_expressions),
//          std::move(group_expressions),
//          SHA1::compute(node->toString())));
//}
//
//ScopedPtr<Task> TableExpressionBuilder::buildJoin(
//    Transaction* ctx,
//    RefPtr<JoinNode> node,
//    QueryBuilder* runtime,
//    TableProvider* tables) {
//  Vector<String> column_names;
//  Vector<ValueExpression> select_expressions;
//
//  for (const auto& slnode : node->selectList()) {
//    select_expressions.emplace_back(
//        runtime->buildValueExpression(ctx, slnode->expression()));
//  }
//
//  Option<ValueExpression> where_expr;
//  if (!node->whereExpression().isEmpty()) {
//    where_expr = std::move(Option<ValueExpression>(
//        runtime->buildValueExpression(ctx, node->whereExpression().get())));
//  }
//
//  Option<ValueExpression> join_cond_expr;
//  if (!node->joinCondition().isEmpty()) {
//    join_cond_expr = std::move(Option<ValueExpression>(
//        runtime->buildValueExpression(ctx, node->joinCondition().get())));
//  }
//
//  auto base_tbl = build(ctx, node->baseTable(), runtime, tables);
//  auto joined_tbl = build(ctx, node->joinedTable(), runtime, tables);
//
//  return mkScoped(
//      new NestedLoopJoin(
//          ctx,
//          node->joinType(),
//          std::move(base_tbl),
//          std::move(joined_tbl),
//          node->inputColumnMap(),
//          node->outputColumns(),
//          std::move(select_expressions),
//          std::move(join_cond_expr),
//          std::move(where_expr)));
//}

//ScopedPtr<Task> TableExpressionBuilder::buildSequentialScan(
//    Transaction* ctx,
//    RefPtr<SequentialScanNode> node,
//    QueryBuilder* runtime,
//    TableProvider* tables) {
//  RAISE(kNotYetImplementedError);
//  //const auto& table_name = node->tableName();
//
//  //{
//  //  auto cols = node->selectedColumns();
//  //  auto tinfo = tables->describe(node->tableName());
//  //  if (tinfo.isEmpty()) {
//  //    RAISEF(kRuntimeError, "table not found: $0", table_name);
//  //  }
//
//  //  for (const auto& c : tinfo.get().columns) {
//  //    cols.erase(c.column_name);
//  //  }
//
//  //  if (!cols.empty()) {
//  //    RAISEF(
//  //        kRuntimeError,
//  //        "column(s) not found: $0",
//  //        StringUtil::join(cols, ", "));
//  //  }
//  //}
//
//  //auto seqscan = tables->buildSequentialScan(ctx, node, );
//  //if (seqscan.isEmpty()) {
//  //  RAISEF(kRuntimeError, "table not found: $0", table_name);
//  //}
//
//  //return std::move(seqscan.get());
//}
//
//ScopedPtr<Task> TableExpressionBuilder::buildSubquery(
//    Transaction* ctx,
//    RefPtr<SubqueryNode> node,
//    QueryBuilder* runtime,
//    TableProvider* tables) {
//  Vector<String> column_names;
//  Vector<ValueExpression> select_expressions;
//  Option<ValueExpression> where_expr;
//
//  if (!node->whereExpression().isEmpty()) {
//    where_expr = std::move(Option<ValueExpression>(
//        runtime->buildValueExpression(ctx, node->whereExpression().get())));
//  }
//
//  for (const auto& slnode : node->selectList()) {
//    select_expressions.emplace_back(
//        runtime->buildValueExpression(ctx, slnode->expression()));
//  }
//
//  return mkScoped(new SubqueryExpression(
//      ctx,
//      node->outputColumns(),
//      std::move(select_expressions),
//      std::move(where_expr),
//      build(ctx, node->subquery(), runtime, tables)));
//}
//
////ScopedPtr<Task> TableExpressionBuilder::buildGroupMerge(
////    Transaction* ctx,
////    RefPtr<GroupByMergeNode> node,
////    QueryBuilder* runtime,
////    TableProvider* tables) {
////  Vector<ScopedPtr<GroupByExpression>> shards;
////
////  for (const auto& table : node->inputTables()) {
////    auto shard = build(ctx, table, runtime, tables);
////    auto tshard = dynamic_cast<GroupByExpression*>(shard.get());
////    if (tshard) {
////      shard.release();
////    } else {
////      RAISE(kIllegalStateError, "GROUP MERGE subexpression must be a GROUP BY");
////    }
////
////    shards.emplace_back(mkScoped(tshard));
////  }
////
////  return mkScoped(new GroupByMerge(std::move(shards)));
////}
//
//ScopedPtr<Task> TableExpressionBuilder::buildUnion(
//    Transaction* ctx,
//    RefPtr<UnionNode> node,
//    QueryBuilder* runtime,
//    TableProvider* tables) {
//  Vector<ScopedPtr<Task>> union_tables;
//
//  for (const auto& table : node->inputTables()) {
//    union_tables.emplace_back(build(ctx, table, runtime, tables));
//  }
//
//  return mkScoped(new Union(std::move(union_tables)));
//}
//
//ScopedPtr<Task> TableExpressionBuilder::buildLimit(
//    Transaction* ctx,
//    RefPtr<LimitNode> node,
//    QueryBuilder* runtime,
//    TableProvider* tables) {
//  return mkScoped(
//      new LimitClause(
//          node->limit(),
//          node->offset(),
//          build(ctx, node->inputTable(), runtime, tables)));
//}
//
//ScopedPtr<Task> TableExpressionBuilder::buildSelectExpression(
//    Transaction* ctx,
//    RefPtr<SelectExpressionNode> node,
//    QueryBuilder* runtime,
//    TableProvider* tables) {
//  Vector<String> column_names;
//  Vector<ValueExpression> select_expressions;
//
//  for (const auto& slnode : node->selectList()) {
//    column_names.emplace_back(slnode->columnName());
//
//    select_expressions.emplace_back(
//        runtime->buildValueExpression(ctx, slnode->expression()));
//  }
//
//  return mkScoped(new SelectExpression(
//      ctx,
//      column_names,
//      std::move(select_expressions)));
//}
//
//ScopedPtr<Task> TableExpressionBuilder::buildDescribeTableStatment(
//    Transaction* ctx,
//    RefPtr<DescribeTableNode> node,
//    QueryBuilder* runtime,
//    TableProvider* tables) {
//  auto table_info = tables->describe(node->tableName());
//  if (table_info.isEmpty()) {
//    RAISEF(kNotFoundError, "table not found: $0", node->tableName());
//  }
//
//  return mkScoped(new DescribeTableStatement(table_info.get()));
//}
//
////ScopedPtr<Task> TableExpressionBuilder::buildRemoteAggregate(
////    Transaction* ctx,
////    RefPtr<RemoteAggregateNode> node,
////    QueryBuilder* runtime) {
////
////  Vector<String> column_names;
////  Vector<ValueExpression> select_expressions;
////
////  for (const auto& slnode : node->selectList()) {
////    column_names.emplace_back(slnode->columnName());
////
////    select_expressions.emplace_back(
////        runtime->buildValueExpression(ctx, slnode->expression()));
////  }
////
////  return mkScoped(
////      new RemoteGroupBy(
////          ctx,
////          column_names,
////          std::move(select_expressions),
////          node->remoteAggregateParams(),
////          node->remoteExecuteFunction()));
////}
//
} // namespace csql
