/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_SQL_RUNTIME_H
#define _FNORDMETRIC_SQL_RUNTIME_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include <chartsql/parser/parser.h>
#include <chartsql/qtree/ChartStatementNode.h>
#include <chartsql/runtime/queryplan.h>
#include <chartsql/runtime/queryplanbuilder.h>
#include <chartsql/runtime/symboltable.h>
#include <chartsql/runtime/ValueExpressionBuilder.h>
#include <chartsql/runtime/TableExpressionBuilder.h>
#include <chartsql/runtime/ResultFormat.h>
#include <chartsql/runtime/charts/ChartStatement.h>

namespace csql {

class Runtime {
public:
  typedef
      Function<RefPtr<QueryTreeNode> (RefPtr<QueryTreeNode> query)>
      QueryRewriteFn;

  Runtime();

  void executeQuery(
      RefPtr<QueryPlan> query_plan,
      RefPtr<ResultFormat> result_format);

  SValue evaluateStaticExpression(const String& expr);
  SValue evaluateStaticExpression(ASTNode* expr);

  RefPtr<QueryPlan> parseAndBuildQueryPlan(
      const String& query,
      RefPtr<TableProvider> tables,
      QueryRewriteFn query_rewrite_fn);

  ScopedPtr<ValueExpression> buildValueExpression(
      RefPtr<ValueExpressionNode> expression);

  ScopedPtr<TableExpression> buildTableExpression(
      RefPtr<TableExpressionNode> expression,
      RefPtr<TableProvider> tables);

  ScopedPtr<ChartStatement> buildChartStatement(
      RefPtr<ChartStatementNode> node,
      RefPtr<TableProvider> tables);

  void registerFunction(const String& name, SFunction fn);

protected:
  SymbolTable symbol_table_;
  QueryPlanBuilder query_plan_builder_;
  ValueExpressionBuilder scalar_exp_builder_;
  TableExpressionBuilder table_exp_builder_;
};

}
#endif
