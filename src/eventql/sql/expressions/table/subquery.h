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
#include <eventql/util/stdtypes.h>
#include <eventql/sql/runtime/defaultruntime.h>
#include <eventql/sql/expressions/table_expression.h>

namespace csql {

class SubqueryExpression : public TableExpression {
public:

  SubqueryExpression(
      Transaction* txn,
      Vector<ValueExpression> select_expressions,
      Option<ValueExpression> where_expr,
      ScopedPtr<TableExpression> input);

  ScopedPtr<ResultCursor> execute() override;

protected:

  bool next(SValue* row, int row_len);

  Transaction* txn_;
  Vector<ValueExpression> select_exprs_;
  Option<ValueExpression> where_expr_;
  ScopedPtr<TableExpression> input_;
  ScopedPtr<ResultCursor> input_cursor_;
  Vector<SValue> buf_;
};

}
