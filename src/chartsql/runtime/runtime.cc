/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/runtime.h>
#include <chartsql/defaults.h>

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

void Runtime::executeQuery(
    const String& query,
    RefPtr<ExecutionStrategy> execution_strategy,
    RefPtr<ResultFormat> result_format) {

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

  auto query_plan = mkRef(
      new QueryPlan(
          statements,
          execution_strategy->tableProvider(),
          query_builder_.get(),
          this));

  /* execute query and format results */
  csql::ExecutionContext context(&tpool_);
  if (!cachedir_.isEmpty()) {
    context.setCacheDir(cachedir_.get());
  }

  result_format->formatResults(query_plan, &context);
}

SValue Runtime::evaluateStaticExpression(ASTNode* expr) {
  auto val_expr = mkRef(query_plan_builder_->buildValueExpression(expr));
  auto compiled = query_builder_->buildValueExpression(val_expr);

  SValue out;
  compiled->evaluate(0, nullptr, &out);
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
  compiled->evaluate(0, nullptr, &out);
  return out;
}

SValue Runtime::evaluateStaticExpression(RefPtr<ValueExpressionNode> expr) {
  auto compiled = query_builder_->buildValueExpression(expr);

  SValue out;
  compiled->evaluate(0, nullptr, &out);
  return out;
}

Option<String> Runtime::cacheDir() const {
  return cachedir_;
}

void Runtime::setCacheDir(const String& cachedir) {
  cachedir_ = Some(cachedir);
}

}
