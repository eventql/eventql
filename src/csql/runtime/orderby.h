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
#include <csql/runtime/TableExpression.h>
#include <csql/qtree/OrderByNode.h>

namespace csql {

class OrderBy : public TableExpression {
public:

  OrderBy(
      Vector<OrderByNode::SortSpec> sort_specs,
      size_t max_output_column_index,
      ScopedPtr<TableExpression> child);

  void prepare(ExecutionContext* context) override;

  void execute(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn) override;

  Vector<String> columnNames() const override;

  size_t numColumns() const override;

protected:
  Vector<OrderByNode::SortSpec> sort_specs_;
  size_t max_output_column_index_;
  ScopedPtr<TableExpression> child_;
};

}
