/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <csql/tasks/Task.h>
#include <csql/runtime/defaultruntime.h>

namespace csql {

class Subquery : public Task {
public:

  Subquery(
      Transaction* txn,
      Vector<ValueExpression> select_expressions,
      Option<ValueExpression> where_expr,
      RowSinkFn output);

  bool onInputRow(
      const TaskID& input_id,
      const SValue* row,
      int row_len) override;

protected:
  Transaction* txn_;
  Vector<ValueExpression> select_exprs_;
  Option<ValueExpression> where_expr_;
  RowSinkFn output_;
};

class SubqueryFactory : public TaskFactory {
public:

  SubqueryFactory(
      Vector<RefPtr<SelectListNode>> select_exprs,
      Option<RefPtr<ValueExpressionNode>> where_expr);

  RefPtr<Task> build(
      Transaction* txn,
      RowSinkFn output) const override;

protected:
  Vector<RefPtr<SelectListNode>> select_exprs_;
  Option<RefPtr<ValueExpressionNode>> where_expr_;
};

}
