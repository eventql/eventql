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
#include <chartsql/qtree/QueryTreeNode.h>
#include <chartsql/qtree/ScalarExpressionNode.h>
#include <chartsql/qtree/SelectListNode.h>

using namespace fnord;

namespace csql {

class GroupByNode : public TableExpressionNode {
public:

  GroupByNode(
      Vector<RefPtr<SelectListNode>> select_list,
      Vector<RefPtr<ScalarExpressionNode>> group_exprs,
      RefPtr<TableExpressionNode> table);

  Vector<RefPtr<SelectListNode>> selectList() const;

protected:
  Vector<RefPtr<SelectListNode>> select_list_;
  Vector<RefPtr<ScalarExpressionNode>> group_exprs_;
  RefPtr<TableExpressionNode> table_;
};

} // namespace csql
