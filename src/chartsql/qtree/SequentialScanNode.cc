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

using namespace fnord;

namespace csql {

SequentialScanNode::SequentialScanNode(
    const String& table_name,
    Vector<RefPtr<SelectListNode>> select_list,
    Option<RefPtr<ValueExpressionNode>> where_expr) :
    table_name_(table_name),
    select_list_(select_list),
    where_expr_(where_expr),
    aggr_strategy_(AggregationStrategy::NO_AGGREGATION) {}

const String& SequentialScanNode::tableName() const {
  return table_name_;
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

} // namespace csql
