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

using namespace stx;

namespace csql {

class UnionNode : public TableExpressionNode {
public:

  UnionNode(const UnionNode& other);
  UnionNode(Vector<RefPtr<QueryTreeNode>> tables);

  Vector<RefPtr<QueryTreeNode>> inputTables() const;

  Vector<String> outputColumns() const override;

  Vector<QualifiedColumn> allColumns() const override;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toString() const override;

  size_t getColumnIndex(
      const String& column_name,
      bool allow_add = false) override;

  Vector<TaskID> build(Transaction* txn, TaskDAG* tree) const override;

protected:
  Vector<RefPtr<QueryTreeNode>> tables_;
};

} // namespace csql
