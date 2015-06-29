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
    RefPtr<TableExpressionNode> table) :
    select_list_(select_list),
    group_exprs_(group_exprs),
    table_(table) {}

Vector<RefPtr<SelectListNode>> GroupByNode::selectList() const {
  return select_list_;
}

Vector<RefPtr<ValueExpressionNode>> GroupByNode::groupExpressions() const {
  return group_exprs_;
}

RefPtr<TableExpressionNode> GroupByNode::inputTable() const {
  return table_;
}

} // namespace csql
