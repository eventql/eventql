/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/runtime.h>
#include <csql/runtime/groupby.h>
#include <csql/defaults.h>

namespace csql {

RefPtr<Runtime> Runtime::getDefaultRuntime() {
  auto symbols = mkRef(new SymbolTable());
  installDefaultSymbols(symbols.get());

  return new Runtime(
      symbols,
      new QueryBuilder(
          new ValueExpressionBuilder(symbols.get()),
          new TableExpressionBuilder()),
      new QueryPlanBuilder(symbols.get()));
}

Runtime::Runtime(
    RefPtr<SymbolTable> symbol_table,
    RefPtr<QueryBuilder> query_builder,
    RefPtr<QueryPlanBuilder> query_plan_builder) :
    symbol_table_(symbol_table),
    query_builder_(query_builder),
    query_plan_builder_(query_plan_builder) {}

RefPtr<QueryPlan> Runtime::buildQueryPlan(
    const String& query,
    RefPtr<ExecutionStrategy> execution_strategy) {
  /* parse query */
  csql::Parser parser;
  parser.parse(query.data(), query.size());

  /* build query plan */
  auto statements = query_plan_builder_->build(
      parser.getStatements(),
      execution_strategy->tableProvider());

  for (auto& stmt : statements) {
    stmt = execution_strategy->rewriteQueryTree(stmt);
  }

  return mkRef(
      new QueryPlan(
          statements,
          execution_strategy->tableProvider(),
          query_builder_.get(),
          this));
}

void Runtime::executeQuery(
    const String& query,
    RefPtr<ExecutionStrategy> execution_strategy,
    RefPtr<ResultFormat> result_format) {
  auto query_plan = buildQueryPlan(query, execution_strategy);

  /* execute query and format results */
  csql::ExecutionContext context(&tpool_);
  if (!cachedir_.isEmpty()) {
    context.setCacheDir(cachedir_.get());
  }

  for (int i = 0; i < query_plan->numStatements(); ++i) {
    query_plan->getStatement(i)->prepare(&context);
  }

  result_format->formatResults(query_plan, &context);
}

void Runtime::executeStatement(
    Statement* statement,
    ResultList* result) {
  auto table_expr = dynamic_cast<TableExpression*>(statement);
  if (!table_expr) {
    RAISE(kRuntimeError, "statement must be a table expression");
  }

  result->addHeader(table_expr->columnNames());
  executeStatement(
      table_expr,
      [result] (int argc, const csql::SValue* argv) -> bool {
    result->addRow(argv, argc);
    return true;
  });
}

void Runtime::executeStatement(
    TableExpression* statement,
    Function<bool (int argc, const SValue* argv)> fn) {
  csql::ExecutionContext context(&tpool_);
  if (!cachedir_.isEmpty()) {
    context.setCacheDir(cachedir_.get());
  }

  statement->prepare(&context);
  statement->execute(&context, fn);
}

void Runtime::executeAggregate(
    const RemoteAggregateParams& query,
    RefPtr<ExecutionStrategy> execution_strategy,
    OutputStream* os) {
  Vector<RefPtr<SelectListNode>> outer_select_list;
  for (const auto& e : query.aggregate_expression_list()) {
    csql::Parser parser;
    parser.parseValueExpression(
        e.expression().data(),
        e.expression().size());

    auto stmts = parser.getStatements();
    if (stmts.size() != 1) {
      RAISE(kIllegalArgumentError);
    }

    auto slnode = mkRef(
        new SelectListNode(
            query_plan_builder_->buildValueExpression(stmts[0])));

    if (e.has_alias()) {
      slnode->setAlias(e.alias());
    }

    outer_select_list.emplace_back(slnode);
  }

  Vector<RefPtr<SelectListNode>> inner_select_list;
  for (const auto& e : query.select_expression_list()) {
    csql::Parser parser;
    parser.parseValueExpression(
        e.expression().data(),
        e.expression().size());

    auto stmts = parser.getStatements();
    if (stmts.size() != 1) {
      RAISE(kIllegalArgumentError);
    }

    auto slnode = mkRef(
        new SelectListNode(
            query_plan_builder_->buildValueExpression(stmts[0])));

    if (e.has_alias()) {
      slnode->setAlias(e.alias());
    }

    inner_select_list.emplace_back(slnode);
  }

  Vector<RefPtr<ValueExpressionNode>> group_exprs;
  for (const auto& e : query.group_expression_list()) {
    csql::Parser parser;
    parser.parseValueExpression(e.data(), e.size());

    auto stmts = parser.getStatements();
    if (stmts.size() != 1) {
      RAISE(kIllegalArgumentError);
    }

    group_exprs.emplace_back(
        query_plan_builder_->buildValueExpression(stmts[0]));
  }

  Option<RefPtr<ValueExpressionNode>> where_expr;
  if (query.has_where_expression()) {
    csql::Parser parser;
    parser.parseValueExpression(
        query.where_expression().data(),
        query.where_expression().size());

    auto stmts = parser.getStatements();
    if (stmts.size() != 1) {
      RAISE(kIllegalArgumentError);
    }

    where_expr = Some(mkRef(
        query_plan_builder_->buildValueExpression(stmts[0])));
  }

  auto qtree = mkRef(
      new GroupByNode(
          outer_select_list,
          group_exprs,
          new SequentialScanNode(
                query.table_name(),
                inner_select_list,
                where_expr,
                (AggregationStrategy) query.aggregation_strategy())));

  auto expr = query_builder_->buildTableExpression(
      qtree.get(),
      execution_strategy->tableProvider(),
      this);

  auto group_expr = dynamic_cast<GroupByExpression*>(expr.get());
  if (!group_expr) {
    RAISE(kIllegalStateError);
  }

  csql::ExecutionContext context(&tpool_);
  if (!cachedir_.isEmpty()) {
    context.setCacheDir(cachedir_.get());
  }

  group_expr->executeRemote(&context, os);
}

SValue Runtime::evaluateStaticExpression(ASTNode* expr) {
  auto val_expr = mkRef(query_plan_builder_->buildValueExpression(expr));
  auto compiled = query_builder_->buildValueExpression(val_expr);

  SValue out;
  VM::evaluate(compiled.program(), 0, nullptr, &out);
  return out;
}

SValue Runtime::evaluateStaticExpression(const String& expr) {
  csql::Parser parser;
  parser.parseValueExpression(expr.data(), expr.size());

  auto stmts = parser.getStatements();
  if (stmts.size() != 1) {
    RAISE(
        kParseError,
        "static expression must consist of exactly one statement");
  }

  auto val_expr = mkRef(query_plan_builder_->buildValueExpression(stmts[0]));
  auto compiled = query_builder_->buildValueExpression(val_expr);

  SValue out;
  VM::evaluate(compiled.program(), 0, nullptr, &out);
  return out;
}

SValue Runtime::evaluateStaticExpression(RefPtr<ValueExpressionNode> expr) {
  auto compiled = query_builder_->buildValueExpression(expr);

  SValue out;
  VM::evaluate(compiled.program(), 0, nullptr, &out);
  return out;
}

Option<String> Runtime::cacheDir() const {
  return cachedir_;
}

void Runtime::setCacheDir(const String& cachedir) {
  cachedir_ = Some(cachedir);
}

RefPtr<QueryBuilder> Runtime::queryBuilder() const {
  return query_builder_;
}

RefPtr<QueryPlanBuilder> Runtime::queryPlanBuilder() const {
  return query_plan_builder_;
}

TaskScheduler* Runtime::scheduler() {
  return &tpool_;
}


}
