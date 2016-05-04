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
#include <csql/qtree/TableExpressionNode.h>
#include <csql/qtree/ValueExpressionNode.h>

using namespace stx;

namespace csql {

class OrderByNode : public TableExpressionNode {
public:

  struct SortSpec {
    RefPtr<ValueExpressionNode> expr;
    bool descending; // false == ASCENDING, true == DESCENDING
  };

  OrderByNode(
      Vector<SortSpec> sort_specs,
      RefPtr<QueryTreeNode> table);

  RefPtr<QueryTreeNode> inputTable() const;

  Vector<String> outputColumns() const override;

  Vector<QualifiedColumn> allColumns() const override;

  const Vector<SortSpec>& sortSpecs() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toString() const override;

  size_t getColumnIndex(
      const String& column_name,
      bool allow_add = false) override;

protected:
  Vector<SortSpec> sort_specs_;
  size_t max_output_column_index_;
  RefPtr<QueryTreeNode> table_;
};

} // namespace csql
