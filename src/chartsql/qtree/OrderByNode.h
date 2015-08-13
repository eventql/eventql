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
#include <chartsql/qtree/TableExpressionNode.h>

using namespace stx;

namespace csql {

class OrderByNode : public TableExpressionNode {
public:

  struct SortSpec {
    size_t column;
    bool descending; // false == ASCENDING, true == DESCENDING
  };

  OrderByNode(
      Vector<SortSpec> sort_specs,
      size_t max_output_column_index,
      RefPtr<QueryTreeNode> table);

  RefPtr<QueryTreeNode> inputTable() const;

  const Vector<SortSpec>& sortSpecs() const;

  size_t maxOutputColumnIndex() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toString() const override;

protected:
  Vector<SortSpec> sort_specs_;
  size_t max_output_column_index_;
  RefPtr<QueryTreeNode> table_;
};

} // namespace csql
