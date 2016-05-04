/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/qtree/LimitNode.h>
#include <csql/tasks/limit.h>

using namespace stx;

namespace csql {

LimitNode::LimitNode(
    size_t limit,
    size_t offset,
    RefPtr<QueryTreeNode> table) :
    limit_(limit),
    offset_(offset),
    table_(table) {
  addChild(&table_);
}

RefPtr<QueryTreeNode> LimitNode::inputTable() const {
  return table_;
}

Vector<String> LimitNode::outputColumns() const {
  return table_.asInstanceOf<TableExpressionNode>()->outputColumns();
}

Vector<QualifiedColumn> LimitNode::allColumns() const {
  return table_.asInstanceOf<TableExpressionNode>()->allColumns();
}

size_t LimitNode::getColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  return table_.asInstanceOf<TableExpressionNode>()->getColumnIndex(
      column_name,
      allow_add);
}

Vector<TaskID> LimitNode::build(Transaction* txn, TaskDAG* tree) const {
  auto input = table_.asInstanceOf<TableExpressionNode>()->build(txn, tree);

  TaskIDList output;
  auto out_task = mkRef(new TaskDAGNode(new LimitFactory(limit_, offset_)));
  for (const auto& in_task_id : input) {
    TaskDAGNode::Dependency dep;
    dep.task_id = in_task_id;
    out_task->addDependency(dep);
  }
  output.emplace_back(tree->addTask(out_task));

  return output;
}

RefPtr<QueryTreeNode> LimitNode::deepCopy() const {
  return new LimitNode(
      limit_,
      offset_,
      table_->deepCopy().asInstanceOf<QueryTreeNode>());
}

size_t LimitNode::limit() const {
  return limit_;
}

size_t LimitNode::offset() const {
  return offset_;
}

String LimitNode::toString() const {
  return StringUtil::format(
      "(limit $0 $1 (subexpr $2))",
      limit_,
      offset_,
      table_->toString());
}


} // namespace csql
