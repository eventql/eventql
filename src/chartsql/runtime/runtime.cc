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
#include <chartsql/runtime/charts/drawstatement.h>

namespace csql {

Runtime::Runtime() :
    query_plan_builder_(&symbol_table_),
    scalar_exp_builder_(&symbol_table_) {}

void Runtime::executeQuery(
    const String& query,
    RefPtr<TableProvider> table_provider,
    RefPtr<ResultFormat> result_format) {

  auto query_plan = parseAndBuildQueryPlan(query, table_provider);
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

RefPtr<QueryPlan> Runtime::parseAndBuildQueryPlan(
    const String& query,
    RefPtr<TableProvider> tables) {
  Vector<RefPtr<QueryTreeNode>> statements;
  Vector<ChartStatement> charts;

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  for (auto stmt : query_plan_builder_.build(parser.getStatements(), tables)) {
    for (const auto& fn : qtree_rewrite_fns_) {
      stmt = fn(stmt);
    }

    statements.emplace_back(stmt);
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
  return table_exp_builder_.build(node.get(), this, tables.get());
}

ScopedPtr<ChartStatement> Runtime::buildChartStatement(
    RefPtr<ChartStatementNode> node,
    RefPtr<TableProvider> tables) {
  Vector<ScopedPtr<DrawStatement>> draw_statements;

  for (size_t i = 0; i < node->numChildren(); ++i) {
    Vector<ScopedPtr<TableExpression>> union_tables;

    auto draw_stmt_node = node->child(i).asInstanceOf<DrawStatementNode>();
    for (const auto& table : draw_stmt_node->inputTables()) {
      union_tables.emplace_back(
          table_exp_builder_.build(table, this, tables.get()));
    }

    draw_statements.emplace_back(
        new DrawStatement(draw_stmt_node, std::move(union_tables), this));
  }

  return mkScoped(new ChartStatement(std::move(draw_statements)));
}

void Runtime::registerFunction(const String& name, SFunction fn) {
  symbol_table_.registerFunction(name, fn);
}

}
