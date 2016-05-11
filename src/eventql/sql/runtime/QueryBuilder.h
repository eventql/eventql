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
#include <eventql/util/stdtypes.h>
#include <eventql/sql/qtree/ChartStatementNode.h>
#include <eventql/sql/runtime/ValueExpressionBuilder.h>
#include <eventql/sql/runtime/charts/ChartStatement.h>
#include <eventql/sql/runtime/ValueExpression.h>

using namespace stx;

namespace csql {

class QueryBuilder : public RefCounted {
public:

  QueryBuilder(
      RefPtr<ValueExpressionBuilder> scalar_exp_builder);

  ValueExpression buildValueExpression(
      Transaction* ctx,
      RefPtr<ValueExpressionNode> expression);

protected:
  RefPtr<ValueExpressionBuilder> scalar_exp_builder_;
};

} // namespace csql
