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
#include <eventql/sql/qtree/TableExpressionNode.h>
#include <eventql/sql/qtree/ValueExpressionNode.h>
#include <eventql/sql/qtree/SelectListNode.h>

using namespace stx;

namespace csql {

class GroupByNode : public TableExpressionNode {
public:

  GroupByNode(
      Vector<RefPtr<SelectListNode>> select_list,
      Vector<RefPtr<ValueExpressionNode>> group_exprs,
      RefPtr<QueryTreeNode> table);

  GroupByNode(const GroupByNode& other);

  Vector<RefPtr<SelectListNode>> selectList() const;

  Vector<String> outputColumns() const override;

  Vector<QualifiedColumn> allColumns() const override;

  Vector<RefPtr<ValueExpressionNode>> groupExpressions() const;

  RefPtr<QueryTreeNode> inputTable() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toString() const override;

  size_t getColumnIndex(
      const String& column_name,
      bool allow_add = false) override;

protected:
  Vector<RefPtr<SelectListNode>> select_list_;
  Vector<String> column_names_;
  Vector<RefPtr<ValueExpressionNode>> group_exprs_;
  RefPtr<QueryTreeNode> table_;
};

} // namespace csql
