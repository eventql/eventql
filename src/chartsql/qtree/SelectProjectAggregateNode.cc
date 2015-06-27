/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/SelectProjectAggregateNode.h>

using namespace fnord;

namespace csql {

SelectProjectAggregateNode::SelectProjectAggregateNode(
    const String& table_name,
    Vector<RefPtr<ScalarExpressionNode>> select_list,
    RefPtr<ScalarExpressionNode> where_expr) :
    table_name_(table_name),
    select_list_(select_list),
    where_expr_(where_expr),
    expand_nested_records_(true) {}

const String& SelectProjectAggregateNode::tableName() const {
  return table_name_;
}

Vector<RefPtr<ScalarExpressionNode>> SelectProjectAggregateNode::selectList()
    const {
  return select_list_;
}

bool SelectProjectAggregateNode::expandNestedRecords() const {
  return expand_nested_records_;
}

void SelectProjectAggregateNode::setExpandNestedRecords(bool expand) {
  expand_nested_records_ = expand;
}

} // namespace csql
