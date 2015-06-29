/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_SQL_DEFAULTRUNTIME_H
#define _FNORDMETRIC_SQL_DEFAULTRUNTIME_H
#include <chartsql/runtime/runtime.h>
#include <chartsql/runtime/symboltable.h>
#include <chartsql/runtime/ValueExpressionBuilder.h>
#include <chartsql/runtime/TableExpressionBuilder.h>
#include <chartsql/runtime/DefaultExecutionPlan.h>

namespace csql {

class DefaultRuntime {
public:

  DefaultRuntime(SymbolTable* symbol_table);

  RefPtr<ExecutionPlan> buildExecutionPlan(
      RefPtr<QueryTreeNode> qtree,
      TableRepository* tables);

  ScopedPtr<ValueExpression> buildValueExpression(
      RefPtr<ValueExpressionNode> expression);

  ScopedPtr<TableExpression> buildTableExpression(
      RefPtr<TableExpressionNode> expression,
      TableRepository* tables);

protected:
  ValueExpressionBuilder scalar_exp_builder_;
  TableExpressionBuilder table_exp_builder_;
};

}
#endif
