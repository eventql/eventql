/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/stdtypes.h>
#include <chartsql/qtree/ChartStatementNode.h>
#include <chartsql/runtime/ValueExpressionBuilder.h>
#include <chartsql/runtime/TableExpressionBuilder.h>
#include <chartsql/runtime/charts/ChartStatement.h>

using namespace fnord;

namespace csql {

class QueryBuilder {
public:

  QueryBuilder(
      ValueExpressionBuilder scalar_exp_builder,
      TableExpressionBuilder table_exp_builder);

  ScopedPtr<ValueExpression> buildValueExpression(
      RefPtr<ValueExpressionNode> expression);

  ScopedPtr<TableExpression> buildTableExpression(
      RefPtr<TableExpressionNode> expression,
      RefPtr<TableProvider> tables);

  ScopedPtr<ChartStatement> buildChartStatement(
      RefPtr<ChartStatementNode> node,
      RefPtr<TableProvider> tables);

protected:
  ValueExpressionBuilder scalar_exp_builder_;
  TableExpressionBuilder table_exp_builder_;
};

} // namespace csql
