/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/SequentialScanNode.h>

using namespace stx;

namespace csql {

SequentialScanNode::SequentialScanNode(
    const String& table_name,
    Vector<RefPtr<SelectListNode>> select_list,
    Option<RefPtr<ValueExpressionNode>> where_expr) :
    table_name_(table_name),
    select_list_(select_list),
    where_expr_(where_expr),
    aggr_strategy_(AggregationStrategy::NO_AGGREGATION) {}

SequentialScanNode::SequentialScanNode(
    const SequentialScanNode& other) :
    table_name_(other.table_name_),
    aggr_strategy_(other.aggr_strategy_) {
  for (const auto& e : other.select_list_) {
    select_list_.emplace_back(e->deepCopyAs<SelectListNode>());
  }

  if (!other.where_expr_.isEmpty()) {
    where_expr_ = Some(
        other.where_expr_.get()->deepCopyAs<ValueExpressionNode>());
  }
}

Option<RefPtr<ValueExpressionNode>> SequentialScanNode::whereExpression() const {
  return where_expr_;
}

const String& SequentialScanNode::tableName() const {
  return table_name_;
}

void SequentialScanNode::setTableName(const String& table_name) {
  table_name_ = table_name;
}

Vector<RefPtr<SelectListNode>> SequentialScanNode::selectList()
    const {
  return select_list_;
}

AggregationStrategy SequentialScanNode::aggregationStrategy() const {
  return aggr_strategy_;
}

void SequentialScanNode::setAggregationStrategy(AggregationStrategy strategy) {
  aggr_strategy_ = strategy;
}

RefPtr<QueryTreeNode> SequentialScanNode::deepCopy() const {
  return new SequentialScanNode(*this);
}

} // namespace csql
