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
#include <csql/runtime/TableExpression.h>
#include <csql/runtime/ValueExpression.h>
#include <csql/qtree/OrderByNode.h>

namespace csql {

class OrderBy : public TableExpression {
public:

  struct SortExpr {
    ValueExpression expr;
    bool descending; // false == ASCENDING, true == DESCENDING
  };

  OrderBy(
      Transaction* ctx,
      Vector<SortExpr> sort_specs,
      ScopedPtr<TableExpression> child);

  void prepare(ExecutionContext* context) override;

  void execute(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn) override;

  Vector<String> columnNames() const override;

  size_t numColumns() const override;

protected:
  Transaction* ctx_;
  Vector<SortExpr> sort_specs_;
  ScopedPtr<TableExpression> child_;
};

}
