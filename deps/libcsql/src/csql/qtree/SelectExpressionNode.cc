/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/qtree/SelectExpressionNode.h>
#include <csql/tasks/select.h>

using namespace stx;

namespace csql {

SelectExpressionNode::SelectExpressionNode(
    Vector<RefPtr<SelectListNode>> select_list) :
    select_list_(select_list) {
  for (const auto& sl : select_list_) {
    column_names_.emplace_back(sl->columnName());
  }
}

Vector<RefPtr<SelectListNode>> SelectExpressionNode::selectList()
    const {
  return select_list_;
}

Vector<String> SelectExpressionNode::outputColumns() const {
  return column_names_;
}

Vector<QualifiedColumn> SelectExpressionNode::allColumns() const {
  String qualifier;

  Vector<QualifiedColumn> cols;
  for (const auto& c : column_names_) {
    QualifiedColumn qc;
    qc.short_name = c;
    qc.qualified_name = qualifier + c;
    cols.emplace_back(qc);
  }

  return cols;
}

size_t SelectExpressionNode::getColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  for (int i = 0; i < column_names_.size(); ++i) {
    if (column_names_[i] == column_name) {
      return i;
    }
  }

  return -1;
}

Vector<TaskID> SelectExpressionNode::build(Transaction* txn, TaskDAG* tree) const {
  TaskIDList output;
  auto out_task = mkRef(new TaskDAGNode(new SelectFactory(selectList())));
  output.emplace_back(tree->addTask(out_task));
  return output;
}

RefPtr<QueryTreeNode> SelectExpressionNode::deepCopy() const {
  Vector<RefPtr<SelectListNode>> args;
  for (const auto& arg : select_list_) {
    args.emplace_back(arg->deepCopyAs<SelectListNode>());
  }

  return new SelectExpressionNode(args);
}

String SelectExpressionNode::toString() const {
  String str = "(select-expr";

  for (const auto& e : select_list_) {
    str += " " + e->toString();
  }

  str += ")";
  return str;
}

} // namespace csql
