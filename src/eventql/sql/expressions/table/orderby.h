/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/sql/Transaction.h>
#include <eventql/sql/runtime/ValueExpression.h>
#include <eventql/sql/qtree/OrderByNode.h>
#include <eventql/sql/expressions/table_expression.h>

namespace csql {

class OrderByExpression : public TableExpression {
public:

  struct SortExpr {
    ValueExpression expr;
    bool descending; // false == ASCENDING, true == DESCENDING
  };

  OrderByExpression(
      Transaction* txn,
      Vector<SortExpr> sort_specs,
      ScopedPtr<TableExpression> input);

  ScopedPtr<ResultCursor> execute() override;

protected:

  bool next(SValue* row, int row_len);

  Transaction* txn_;
  Vector<SortExpr> sort_specs_;
  ScopedPtr<TableExpression> input_;
  ScopedPtr<ResultCursor> input_cursor_;
  size_t pos_;
  Vector<Vector<SValue>> rows_;
};

}
