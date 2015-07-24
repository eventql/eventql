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
#include <fnord/stdtypes.h>
#include <chartsql/qtree/TableExpressionNode.h>
#include <chartsql/qtree/ValueExpressionNode.h>
#include <chartsql/qtree/SelectListNode.h>

using namespace fnord;

namespace csql {

class GroupByNode : public TableExpressionNode {
public:

  GroupByNode(
      Vector<RefPtr<SelectListNode>> select_list,
      Vector<RefPtr<ValueExpressionNode>> group_exprs,
      RefPtr<QueryTreeNode> table);

  Vector<RefPtr<SelectListNode>> selectList() const;

  Vector<RefPtr<ValueExpressionNode>> groupExpressions() const;

  RefPtr<QueryTreeNode> inputTable() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

protected:
  Vector<RefPtr<SelectListNode>> select_list_;
  Vector<RefPtr<ValueExpressionNode>> group_exprs_;
  RefPtr<QueryTreeNode> table_;
};

} // namespace csql
