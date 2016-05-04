/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/qtree/JoinNode.h>
#include <csql/qtree/ColumnReferenceNode.h>
#include <csql/qtree/QueryTreeUtil.h>
#include <csql/tasks/nested_loop_join.h>

using namespace stx;

namespace csql {

JoinNode::JoinNode(
    JoinType join_type,
    RefPtr<QueryTreeNode> base_table,
    RefPtr<QueryTreeNode> joined_table,
    Vector<RefPtr<SelectListNode>> select_list,
    Option<RefPtr<ValueExpressionNode>> where_expr,
    Option<RefPtr<ValueExpressionNode>> join_cond) :
    join_type_(join_type),
    base_table_(base_table),
    joined_table_(joined_table),
    select_list_(select_list),
    where_expr_(where_expr),
    join_cond_(join_cond) {
  for (const auto& sl : select_list_) {
    column_names_.emplace_back(sl->columnName());
  }

  addChild(&base_table_);
  addChild(&joined_table_);
}

JoinNode::JoinNode(
    const JoinNode& other) :
    join_type_(other.join_type_),
    base_table_(other.base_table_->deepCopy()),
    joined_table_(other.joined_table_->deepCopy()),
    input_map_(other.input_map_),
    column_names_(other.column_names_),
    join_cond_(other.join_cond_) {
  for (const auto& e : other.select_list_) {
    select_list_.emplace_back(e->deepCopyAs<SelectListNode>());
  }

  if (!other.where_expr_.isEmpty()) {
    where_expr_ = Some(
        other.where_expr_.get()->deepCopyAs<ValueExpressionNode>());
  }

  addChild(&base_table_);
  addChild(&joined_table_);
}

JoinType JoinNode::joinType() const {
  return join_type_;
}

RefPtr<QueryTreeNode> JoinNode::baseTable() const {
  return base_table_;
}

RefPtr<QueryTreeNode> JoinNode::joinedTable() const {
  return joined_table_;
}

Vector<RefPtr<SelectListNode>> JoinNode::selectList() const {
  return select_list_;
}

Vector<String> JoinNode::outputColumns() const {
  return column_names_;
}

Vector<QualifiedColumn> JoinNode::allColumns() const {
  Vector<QualifiedColumn> cols;

  for (const auto& c :
      base_table_.asInstanceOf<TableExpressionNode>()->allColumns()) {
    cols.emplace_back(c);
  }

  for (const auto& c :
      joined_table_.asInstanceOf<TableExpressionNode>()->allColumns()) {
    cols.emplace_back(c);
  }

  return cols;
}

size_t JoinNode::getColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  for (int i = 0; i < column_names_.size(); ++i) {
    if (column_names_[i] == column_name) {
      return i;
    }
  }

  auto input_idx = getInputColumnIndex(column_name);
  if (input_idx != size_t(-1)) {
    auto slnode = new SelectListNode(new ColumnReferenceNode(input_idx));
    slnode->setAlias(column_name);
    select_list_.emplace_back(slnode);
    return select_list_.size() - 1;
  }

  return -1; // FIXME
}

size_t JoinNode::getInputColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  for (int i = 0; i < input_map_.size(); ++i) {
    if (input_map_[i].column == column_name) {
      return i;
    }
  }

  auto base_table_idx = base_table_
      .asInstanceOf<TableExpressionNode>()
      ->getColumnIndex(column_name, allow_add);

  auto joined_table_idx = joined_table_
      .asInstanceOf<TableExpressionNode>()
      ->getColumnIndex(column_name, allow_add);

  if (base_table_idx != size_t(-1) && joined_table_idx != size_t(-1)) {
    RAISEF(
        kRuntimeError,
        "ambiguous column reference: '$0'",
        column_name);
  }

  if (base_table_idx != size_t(-1)) {
    input_map_.emplace_back(InputColumnRef{
      .column = column_name,
      .table_idx = 0,
      .column_idx = base_table_idx
    });

    return input_map_.size() - 1;
  }

  if (joined_table_idx != size_t(-1)) {
    input_map_.emplace_back(InputColumnRef{
      .column = column_name,
      .table_idx = 1,
      .column_idx = joined_table_idx
    });

    return input_map_.size() - 1;
  }

  return -1;
}

Option<RefPtr<ValueExpressionNode>> JoinNode::whereExpression() const {
  return where_expr_;
}

Option<RefPtr<ValueExpressionNode>> JoinNode::joinCondition() const {
  return join_cond_;
}

Vector<TaskID> JoinNode::build(Transaction* txn, TaskDAG* tree) const {
  auto base_table_tasks =
      base_table_.asInstanceOf<TableExpressionNode>()->build(txn, tree);
  auto joined_table_tasks =
      joined_table_.asInstanceOf<TableExpressionNode>()->build(txn, tree);

  Set<TaskID> input_tasks_idset;
  Set<TaskID> base_table_tasks_idset;
  Set<TaskID> joined_table_tasks_idset;
  for (const auto& task_id : base_table_tasks) {
    input_tasks_idset.emplace(task_id);
    base_table_tasks_idset.emplace(task_id);
  }
  for (const auto& task_id : joined_table_tasks) {
    input_tasks_idset.emplace(task_id);
    joined_table_tasks_idset.emplace(task_id);
  }

  auto out_task = mkRef(new TaskDAGNode(
      new NestedLoopJoinFactory(
          join_type_,
          base_table_tasks_idset,
          joined_table_tasks_idset,
          input_map_,
          selectList(),
          joinCondition(),
          whereExpression())));

  for (const auto& in_task_id : input_tasks_idset) {
    TaskDAGNode::Dependency dep;
    dep.task_id = in_task_id;
    out_task->addDependency(dep);
  }

  TaskIDList output;
  output.emplace_back(tree->addTask(out_task));
  return output;
}

RefPtr<QueryTreeNode> JoinNode::deepCopy() const {
  return new JoinNode(*this);
}

const Vector<JoinNode::InputColumnRef>& JoinNode::inputColumnMap() const {
  return input_map_;
}

String JoinNode::toString() const {
  auto str = StringUtil::format(
      "(join (base-table $0) (joined-table $0) (select-list",
      base_table_->toString(),
      joined_table_->toString());

  for (const auto& e : select_list_) {
    str += " " + e->toString();
  }
  str += ")";

  if (!where_expr_.isEmpty()) {
    str += StringUtil::format(" (where $0)", where_expr_.get()->toString());
  }

  if (!join_cond_.isEmpty()) {
    str += StringUtil::format(" (join-cond $0)", join_cond_.get()->toString());
  }

  str += ")";
  return str;
};

} // namespace csql

