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
#include <stx/stdtypes.h>
#include <csql/qtree/ChartStatementNode.h>
#include <csql/runtime/ValueExpressionBuilder.h>
#include <csql/runtime/TableExpressionBuilder.h>
#include <csql/runtime/charts/ChartStatement.h>
#include <csql/runtime/ValueExpression.h>

using namespace stx;

namespace csql {

class QueryBuilder : public RefCounted {
public:

  QueryBuilder(
      RefPtr<ValueExpressionBuilder> scalar_exp_builder,
      RefPtr<TableExpressionBuilder> table_exp_builder);

  ValueExpression buildValueExpression(
      Transaction* ctx,
      RefPtr<ValueExpressionNode> expression);

  ScopedPtr<Task> buildTableExpression(
      Transaction* ctx,
      RefPtr<TableExpressionNode> expression,
      RefPtr<TableProvider> tables,
      Runtime* runtime);

  ScopedPtr<ChartStatement> buildChartStatement(
      Transaction* ctx,
      RefPtr<ChartStatementNode> node,
      RefPtr<TableProvider> tables,
      Runtime* runtime);

protected:
  RefPtr<ValueExpressionBuilder> scalar_exp_builder_;
  RefPtr<TableExpressionBuilder> table_exp_builder_;
};

} // namespace csql
