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
#include <chartsql/drawstatement.h>

namespace csql {

Runtime::Runtime() :
    query_plan_builder_(&symbol_table_),
    scalar_exp_builder_(&symbol_table_) {}

void Runtime::executeQuery(
    RefPtr<QueryPlan> query_plan,
    RefPtr<ResultFormat> result_format) {
  csql::ExecutionContext context;
  result_format->formatResults(query_plan, &context);
}

RefPtr<QueryPlan> Runtime::parseAndBuildQueryPlan(
    const String& query,
    RefPtr<TableProvider> tables,
    QueryRewriteFn rewrite_fn) {
  Vector<RefPtr<QueryTreeNode>> statements;
  Vector<Vector<ScopedPtr<DrawStatement>>> draw_statements(1);

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  for (auto stmt : parser.getStatements()) {
    stmt->debugPrint();

    switch (stmt->getType()) {
      case ASTNode::T_DRAW:
        fnord::iputs("draw!", 1);
        draw_statements_.back().emplace_back
            (new DrawStatement(stmt.get(), this));
        break;
      case ASTNode::T_SELECT:
        statements.emplace_back(rewrite_fn(query_plan_builder_.build(stmt)));
        break;
      default:
        RAISE(kRuntimeError, "invalid statement");
    }
  }

  return new QueryPlan(statements, tables, this);
}

ScopedPtr<ValueExpression> Runtime::buildValueExpression(
    RefPtr<ValueExpressionNode> node) {
  return scalar_exp_builder_.compile(node);
}

ScopedPtr<TableExpression> Runtime::buildTableExpression(
    RefPtr<TableExpressionNode> node,
    RefPtr<TableProvider> tables) {
  return table_exp_builder_.build(node, this, tables.get());
}

void Runtime::registerFunction(const String& name, SFunction fn) {
  symbol_table_.registerFunction(name, fn);
}

}
