/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/qtree/RemoteAggregateNode.h>
#include <csql/qtree/SequentialScanNode.h>

using namespace stx;

namespace csql {

RemoteAggregateNode::RemoteAggregateNode(RefPtr<GroupByNode> group_by) {
  auto seq_scan = group_by->child(0).asInstanceOf<SequentialScanNode>();

  params_.set_table_name(seq_scan->tableName());
  params_.set_aggregation_strategy((uint64_t) seq_scan->aggregationStrategy());

  for (const auto& e : group_by->groupExpressions()) {
    *params_.add_group_expression_list() = e->toSQL();
  }

  for (const auto& e : group_by->selectList()) {
    auto sl = params_.add_aggregate_expression_list();
    sl->set_expression(e->expression()->toSQL());
    sl->set_alias(e->columnName());
  }

  for (const auto& e : seq_scan->selectList()) {
    auto sl = params_.add_select_expression_list();
    sl->set_expression(e->expression()->toSQL());
    sl->set_alias(e->columnName());
  }

  auto where = seq_scan->whereExpression();
  if (!where.isEmpty()) {
    params_.set_where_expression(where.get()->toSQL());
  }
}

RemoteAggregateNode::RemoteAggregateNode(
    const RemoteAggregateParams& params) :
    params_(params) {}

RefPtr<QueryTreeNode> RemoteAggregateNode::deepCopy() const {
  return new RemoteAggregateNode(params_);
}

String RemoteAggregateNode::toString() const {
  return params_.DebugString();
}

} // namespace csql
