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
#include <stx/stdtypes.h>
#include <csql/Transaction.h>
#include <csql/tasks/Task.h>
#include <csql/runtime/ValueExpression.h>
#include <csql/qtree/OrderByNode.h>

namespace csql {

class OrderBy : public Task {
public:

  struct SortExpr {
    ValueExpression expr;
    bool descending; // false == ASCENDING, true == DESCENDING
  };

  OrderBy(
      Transaction* ctx,
      Vector<SortExpr> sort_specs,
      size_t num_columns,
      HashMap<TaskID, ScopedPtr<ResultCursor>> input);

  bool nextRow(SValue* out, int out_len) override;

protected:
  Transaction* ctx_;
  Vector<SortExpr> sort_specs_;
  size_t num_columns_;
  Vector<Vector<SValue>> rows_;
  ScopedPtr<ResultCursorList> input_;
};

class OrderByFactory : public TaskFactory {
public:

  struct SortExpr {
    RefPtr<ValueExpressionNode> expr;
    bool descending; // false == ASCENDING, true == DESCENDING
  };

  OrderByFactory(
      Vector<SortExpr> sort_specs,
      size_t num_columns);

  RefPtr<Task> build(
      Transaction* txn,
      HashMap<TaskID, ScopedPtr<ResultCursor>> input) const override;

protected:
  Vector<SortExpr> sort_specs_;
  size_t num_columns_;
};

}
