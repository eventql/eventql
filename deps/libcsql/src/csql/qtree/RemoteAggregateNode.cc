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

RemoteAggregateNode::RemoteAggregateNode(
    RefPtr<GroupByNode> group_by,
    RemoteExecuteFn execute_fn) :
    stmt_(group_by),
    execute_fn_(execute_fn) {}

RemoteAggregateNode::RemoteExecuteFn RemoteAggregateNode::remoteExecuteFunction()
    const {
  return execute_fn_;
}

RemoteAggregateParams RemoteAggregateNode::remoteAggregateParams() const {
  RemoteAggregateParams params;

  auto seq_scan = stmt_->child(0).asInstanceOf<SequentialScanNode>();

  params.set_table_name(seq_scan->tableName());
  params.set_aggregation_strategy((uint64_t) seq_scan->aggregationStrategy());

  for (const auto& e : stmt_->groupExpressions()) {
    *params.add_group_expression_list() = e->toSQL();
  }

  for (const auto& e : stmt_->selectList()) {
    auto sl = params.add_aggregate_expression_list();
    sl->set_expression(e->expression()->toSQL());
    sl->set_alias(e->columnName());
  }

  for (const auto& e : seq_scan->selectList()) {
    auto sl = params.add_select_expression_list();
    sl->set_expression(e->expression()->toSQL());
    sl->set_alias(e->columnName());
  }

  auto where = seq_scan->whereExpression();
  if (!where.isEmpty()) {
    params.set_where_expression(where.get()->toSQL());
  }

  return params;
}

Vector<RefPtr<SelectListNode>> RemoteAggregateNode::selectList() const {
  return stmt_->selectList();
}

Vector<String> RemoteAggregateNode::outputColumns() const {
  return stmt_->outputColumns();
}

Vector<QualifiedColumn> RemoteAggregateNode::allColumns() const {
  return stmt_->allColumns();
}

size_t RemoteAggregateNode::getColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  return stmt_->getColumnIndex(column_name, allow_add);
}

Vector<TaskID> RemoteAggregateNode::build(Transaction* txn, TaskDAG* tree) const {
  RAISE(kNotYetImplementedError, "not yet implemented");
}

RefPtr<QueryTreeNode> RemoteAggregateNode::deepCopy() const {
  return new RemoteAggregateNode(stmt_, execute_fn_);
}

String RemoteAggregateNode::toString() const {
  return "<remote-aggregate>";
}

} // namespace csql
