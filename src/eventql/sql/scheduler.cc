/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include "eventql/eventql.h"
#include <eventql/sql/scheduler.h>
#include <eventql/sql/query_plan.h>
#include "eventql/server/session.h"
#include "eventql/db/database.h"
#include "eventql/auth/client_auth.h"

namespace csql {

ScopedPtr<TableExpression> DefaultScheduler::buildLimit(
    Transaction* ctx,
    ExecutionContext* execution_context,
    RefPtr<LimitNode> node) {
  return mkScoped(
      new LimitExpression(
          execution_context,
          node->limit(),
          node->offset(),
          buildTableExpression(
              ctx,
              execution_context,
              node->inputTable().asInstanceOf<TableExpressionNode>())));
}

ScopedPtr<TableExpression> DefaultScheduler::buildSelectExpression(
    Transaction* ctx,
    ExecutionContext* execution_context,
    RefPtr<SelectExpressionNode> node) {
  Vector<ValueExpression> select_expressions;
  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        ctx->getCompiler()->buildValueExpression(ctx, slnode->expression()));
  }

  return mkScoped(new SelectExpression(
      ctx,
      execution_context,
      std::move(select_expressions)));
};

ScopedPtr<TableExpression> DefaultScheduler::buildSubquery(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<SubqueryNode> node) {
  Vector<ValueExpression> select_expressions;
  Option<ValueExpression> where_expr;

  if (!node->whereExpression().isEmpty()) {
    where_expr = std::move(Option<ValueExpression>(
        txn->getCompiler()->buildValueExpression(txn, node->whereExpression().get())));
  }

  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        txn->getCompiler()->buildValueExpression(txn, slnode->expression()));
  }

  return mkScoped(new SubqueryExpression(
      txn,
      execution_context,
      std::move(select_expressions),
      std::move(where_expr),
      buildTableExpression(
          txn,
          execution_context,
          node->subquery().asInstanceOf<TableExpressionNode>())));
}

ScopedPtr<TableExpression> DefaultScheduler::buildOrderByExpression(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<OrderByNode> node) {
  Vector<OrderByExpression::SortExpr> sort_exprs;
  for (const auto& ss : node->sortSpecs()) {
    OrderByExpression::SortExpr se;
    se.descending = ss.descending;
    se.expr = txn->getCompiler()->buildValueExpression(txn, ss.expr);
    sort_exprs.emplace_back(std::move(se));
  }

  return mkScoped(
      new OrderByExpression(
          txn,
          execution_context,
          std::move(sort_exprs),
          buildTableExpression(
              txn,
              execution_context,
              node->inputTable().asInstanceOf<TableExpressionNode>())));
}

ScopedPtr<TableExpression> DefaultScheduler::buildSequentialScan(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<SequentialScanNode> node) {
  const auto& table_name = node->tableName();
  auto table_provider = txn->getTableProvider();

  auto seqscan = table_provider->buildSequentialScan(
      txn,
      execution_context,
      node);

  if (seqscan.isEmpty()) {
    RAISEF(kRuntimeError, "table not found: $0", table_name);
  }

  return std::move(seqscan.get());
}

ScopedPtr<TableExpression> DefaultScheduler::buildGroupByExpression(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<GroupByNode> node) {
  Vector<ValueExpression> select_expressions;
  Vector<ValueExpression> group_expressions;

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
      new GroupByExpression(
          txn,
          execution_context,
          std::move(select_expressions),
          std::move(group_expressions),
          buildTableExpression(
              txn,
              execution_context,
              node->inputTable().asInstanceOf<TableExpressionNode>())));
}

ScopedPtr<TableExpression> DefaultScheduler::buildJoinExpression(
    Transaction* ctx,
    ExecutionContext* execution_context,
    RefPtr<JoinNode> node) {
  Vector<String> column_names;
  Vector<ValueExpression> select_expressions;

  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        ctx->getCompiler()->buildValueExpression(ctx, slnode->expression()));
  }

  Option<ValueExpression> where_expr;
  if (!node->whereExpression().isEmpty()) {
    where_expr = std::move(Option<ValueExpression>(
        ctx->getCompiler()->buildValueExpression(ctx, node->whereExpression().get())));
  }

  Option<ValueExpression> join_cond_expr;
  if (!node->joinCondition().isEmpty()) {
    join_cond_expr = std::move(Option<ValueExpression>(
        ctx->getCompiler()->buildValueExpression(ctx, node->joinCondition().get())));
  }

  return mkScoped(
      new NestedLoopJoin(
          ctx,
          node->joinType(),
          node->inputColumnMap(),
          std::move(select_expressions),
          std::move(join_cond_expr),
          std::move(where_expr),
          buildTableExpression(
              ctx,
              execution_context,
              node->baseTable().asInstanceOf<TableExpressionNode>()),
          buildTableExpression(
              ctx,
              execution_context,
              node->joinedTable().asInstanceOf<TableExpressionNode>())));
}

ScopedPtr<TableExpression> DefaultScheduler::buildTableExpression(
    Transaction* ctx,
    ExecutionContext* execution_context,
    RefPtr<TableExpressionNode> node) {

  if (dynamic_cast<LimitNode*>(node.get())) {
    return buildLimit(ctx, execution_context, node.asInstanceOf<LimitNode>());
  }

  if (dynamic_cast<SelectExpressionNode*>(node.get())) {
    return buildSelectExpression(
        ctx,
        execution_context,
        node.asInstanceOf<SelectExpressionNode>());
  }

  if (dynamic_cast<SubqueryNode*>(node.get())) {
    return buildSubquery(
        ctx,
        execution_context,
        node.asInstanceOf<SubqueryNode>());
  }

  if (dynamic_cast<OrderByNode*>(node.get())) {
    return buildOrderByExpression(
        ctx,
        execution_context,
        node.asInstanceOf<OrderByNode>());
  }

  if (dynamic_cast<SequentialScanNode*>(node.get())) {
    return buildSequentialScan(
        ctx,
        execution_context,
        node.asInstanceOf<SequentialScanNode>());
  }

  if (dynamic_cast<GroupByNode*>(node.get())) {
    return buildGroupByExpression(
        ctx,
        execution_context,
        node.asInstanceOf<GroupByNode>());
  }

  if (dynamic_cast<ShowTablesNode*>(node.get())) {
    return mkScoped(new ShowTablesExpression(ctx));
  }

  if (dynamic_cast<ShowDatabasesNode*>(node.get())) {
    return mkScoped(new ShowDatabasesExpression(ctx));
  }

  if (dynamic_cast<DescribeTableNode*>(node.get())) {
    return mkScoped(new DescribeTableStatement(
        ctx,
        node.asInstanceOf<DescribeTableNode>()->tableName()));
  }

  if (dynamic_cast<DescribePartitionsNode*>(node.get())) {
    return mkScoped(new DescribePartitionsExpression(
        ctx,
        node.asInstanceOf<DescribePartitionsNode>()->tableName()));
  }

  if (dynamic_cast<ClusterShowServersNode*>(node.get())) {
    return mkScoped(new ClusterShowServersExpression(ctx));
  }

  if (dynamic_cast<JoinNode*>(node.get())) {
    return buildJoinExpression(
        ctx,
        execution_context,
        node.asInstanceOf<JoinNode>());
  }

  RAISEF(
      kRuntimeError,
      "cannot figure out how to execute that query, sorry. -- $0",
      node->toString());
};

ScopedPtr<ResultCursor> DefaultScheduler::executeSelect(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<TableExpressionNode> select) {
  return mkScoped(
      new TableExpressionResultCursor(
          buildTableExpression(
              txn,
              execution_context,
              select.asInstanceOf<TableExpressionNode>())));
}

ScopedPtr<ResultCursor> DefaultScheduler::executeDraw(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<ChartStatementNode> node) {
  Vector<Vector<ScopedPtr<csql::TableExpression>>> input_tables;
  Vector<Vector<RefPtr<csql::TableExpressionNode>>> input_table_qtrees;
  for (const auto& draw_stmt_qtree : node->getDrawStatements()) {
    input_tables.emplace_back();
    input_table_qtrees.emplace_back();
    auto draw_stmt = draw_stmt_qtree.asInstanceOf<DrawStatementNode>();
    for (const auto& input_tbl : draw_stmt->inputTables()) {
      input_tables.back().emplace_back(buildTableExpression(
          txn,
          execution_context,
          input_tbl.asInstanceOf<TableExpressionNode>()));

      input_table_qtrees.back().emplace_back(
          input_tbl.asInstanceOf<TableExpressionNode>());
    }
  }

  return mkScoped(
      new TableExpressionResultCursor(
          mkScoped(
              new ChartExpression(
                  txn,
                  node,
                  std::move(input_tables),
                  input_table_qtrees))));
}

ScopedPtr<ResultCursor> DefaultScheduler::executeCreateTable(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<CreateTableNode> create_table) {
  auto res = txn->getTableProvider()->createTable(*create_table);
  if (!res.isSuccess()) {
    RAISE(kRuntimeError, res.message());
  }

  // FIXME return result...
  return mkScoped(new EmptyResultCursor());
}

ScopedPtr<ResultCursor> DefaultScheduler::executeCreateDatabase(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<CreateDatabaseNode> create_database) {
  auto res = txn->getTableProvider()->createDatabase(
      create_database->getDatabaseName());
  if (!res.isSuccess()) {
    RAISE(kRuntimeError, res.message());
  }

  // FIXME return result...
  return mkScoped(new EmptyResultCursor());
}

ScopedPtr<ResultCursor> DefaultScheduler::executeUseDatabase(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<UseDatabaseNode> use_database) {
  auto session = static_cast<eventql::Session*>(txn->getUserData());
  auto dbctx = session->getDatabaseContext();

  try {
    auto ns = dbctx->config_directory->getNamespaceConfig(
        use_database->getDatabaseName());
  } catch (const std::exception& e) {
    RAISE(
          kRuntimeError,
          StringUtil::format(
              "Unknown database $0",
              use_database->getDatabaseName()));
  }

  auto rc = session->changeNamespace(use_database->getDatabaseName());
  if (!rc.isSuccess()) {
    RAISE(kRuntimeError, rc.message());
  }

  // FIXME return result...
  return mkScoped(new EmptyResultCursor());
}

ScopedPtr<ResultCursor> DefaultScheduler::executeDropTable(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<DropTableNode> drop_table) {
  auto res = txn->getTableProvider()->dropTable(
      drop_table->getTableName());
  if (!res.isSuccess()) {
    RAISE(kRuntimeError, res.message());
  }

  // FIXME return result...
  return mkScoped(new EmptyResultCursor());
}

ScopedPtr<ResultCursor> DefaultScheduler::executeInsertInto(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<InsertIntoNode> insert_into) {
  Vector<Pair<String, SValue>> data;
  auto value_specs = insert_into->getValueSpecs();
  for (auto spec : value_specs) {
    auto expr = txn->getCompiler()->buildValueExpression(txn, spec.expr);
    auto program = expr.program();
    if (program->has_aggregate_) {
      RAISE(
          kRuntimeError,
          "insert into expression must not contain aggregation"); //FIXME better msg
    }

    SValue value;
    VM::evaluate(txn, program, 0, nullptr, &value);

    Pair<String, SValue> result;
    result.first = spec.column;
    result.second = value;
    data.emplace_back(result);
  }

  auto res = txn->getTableProvider()->insertRecord(
      insert_into->getTableName(),
      data);

  if (!res.isSuccess()) {
    RAISE(kRuntimeError, res.message());
  }

  // FIXME return result...
  return mkScoped(new EmptyResultCursor());
}

ScopedPtr<ResultCursor> DefaultScheduler::executeInsertJSON(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<InsertJSONNode> insert_json) {
  auto res = txn->getTableProvider()->insertRecord(
      insert_json->getTableName(),
      insert_json->getJSON());

  if (!res.isSuccess()) {
    RAISE(kRuntimeError, res.message());
  }

  // FIXME return result...
  return mkScoped(new EmptyResultCursor());
}

ScopedPtr<ResultCursor> DefaultScheduler::executeAlterTable(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<AlterTableNode> alter_table) {
  auto res = txn->getTableProvider()->alterTable(*alter_table);
  if (!res.isSuccess()) {
    RAISE(kRuntimeError, res.message());
  }

  // FIXME return result...
  return mkScoped(new EmptyResultCursor());
}

ScopedPtr<ResultCursor> DefaultScheduler::execute(
    QueryPlan* query_plan,
    ExecutionContext* execution_context,
    size_t stmt_idx) {
  auto stmt = query_plan->getStatement(stmt_idx);

  if (stmt.isInstanceOf<ChartStatementNode>()) {
    return executeDraw(
        query_plan->getTransaction(),
        execution_context,
        stmt.asInstanceOf<ChartStatementNode>());
  }

  if (stmt.isInstanceOf<CreateTableNode>()) {
    return executeCreateTable(
        query_plan->getTransaction(),
        execution_context,
        stmt.asInstanceOf<CreateTableNode>());
  }

  if (stmt.isInstanceOf<CreateDatabaseNode>()) {
    return executeCreateDatabase(
        query_plan->getTransaction(),
        execution_context,
        stmt.asInstanceOf<CreateDatabaseNode>());
  }

  if (stmt.isInstanceOf<UseDatabaseNode>()) {
    return executeUseDatabase(
        query_plan->getTransaction(),
        execution_context,
        stmt.asInstanceOf<UseDatabaseNode>());
  }

  if (stmt.isInstanceOf<DropTableNode>()) {
    return executeDropTable(
        query_plan->getTransaction(),
        execution_context,
        stmt.asInstanceOf<DropTableNode>());
  }

  if (stmt.isInstanceOf<InsertIntoNode>()) {
    return executeInsertInto(
        query_plan->getTransaction(),
        execution_context,
        stmt.asInstanceOf<InsertIntoNode>());
  }

  if (stmt.isInstanceOf<InsertJSONNode>()) {
    return executeInsertJSON(
        query_plan->getTransaction(),
        execution_context,
        stmt.asInstanceOf<InsertJSONNode>());
  }

  if (stmt.isInstanceOf<AlterTableNode>()) {
    return executeAlterTable(
        query_plan->getTransaction(),
        execution_context,
        stmt.asInstanceOf<AlterTableNode>());
  }

  if (stmt.isInstanceOf<TableExpressionNode>()) {
    return executeSelect(
        query_plan->getTransaction(),
        execution_context,
        stmt.asInstanceOf<TableExpressionNode>());
  }

  RAISEF(
      kRuntimeError,
      "cannot figure out how to execute that statement?!, sorry. -- $0",
      stmt->toString());
};


} // namespace csql
