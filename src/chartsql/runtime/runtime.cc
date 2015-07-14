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

namespace csql {

Runtime::Runtime() :
    query_plan_builder_(&symbol_table_),
    scalar_exp_builder_(&symbol_table_) {}

void Runtime::executeQuery(
    const String& query,
    RefPtr<ExecutionStrategy> execution_strategy,
    RefPtr<ResultFormat> result_format) {

  /* parse query */
  csql::Parser parser;
  parser.parse(query.data(), query.size());

  /* build query plan */
  auto statements = query_plan_builder_.build(
      parser.getStatements(),
      execution_strategy->tableProvider());

  for (auto& stmt : statements) {
    stmt = execution_strategy->rewriteQueryTree(stmt);
  }

  auto query_plan = mkRef(
      new QueryPlan(
          statements,
          execution_strategy->tableProvider(),
          this));

  /* execute query and format results */
  csql::ExecutionContext context;
  result_format->formatResults(query_plan, &context);
}

SValue Runtime::evaluateStaticExpression(ASTNode* expr) {
  auto val_expr = mkRef(query_plan_builder_.buildValueExpression(expr));
  auto compiled = scalar_exp_builder_.compile(val_expr);

  SValue out;
  compiled->evaluate(0, nullptr, &out);
  return out;
}

void Runtime::registerFunction(const String& name, SFunction fn) {
  symbol_table_.registerFunction(name, fn);
}

}
