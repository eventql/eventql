/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/GroupByNode.h>

using namespace fnord;

namespace csql {

GroupByNode::GroupByNode(
    Vector<RefPtr<SelectListNode>> select_list,
    Vector<RefPtr<ValueExpressionNode>> group_exprs,
    RefPtr<QueryTreeNode> table) :
    select_list_(select_list),
    group_exprs_(group_exprs),
    table_(table) {
  addChild(&table_);
}

Vector<RefPtr<SelectListNode>> GroupByNode::selectList() const {
  return select_list_;
}

Vector<RefPtr<ValueExpressionNode>> GroupByNode::groupExpressions() const {
  return group_exprs_;
}

RefPtr<QueryTreeNode> GroupByNode::inputTable() const {
  return table_;
}

RefPtr<QueryTreeNode> GroupByNode::deepCopy() const {
  Vector<RefPtr<SelectListNode>> select_list;
  for (const auto& e : select_list_) {
    select_list.emplace_back(e->deepCopyAs<SelectListNode>());
  }

  Vector<RefPtr<ValueExpressionNode>> group_exprs;
  for (const auto& e : group_exprs_) {
    group_exprs.emplace_back(e->deepCopyAs<ValueExpressionNode>());
  }

  return new GroupByNode(
      select_list,
      group_exprs,
      table_->deepCopyAs<QueryTreeNode>());
}

} // namespace csql
