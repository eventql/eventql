/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/qtree/GroupByMergeNode.h>

using namespace stx;

namespace csql {

GroupByMergeNode::GroupByMergeNode(const GroupByMergeNode& other) {
  for (const auto& tbl : other.tables_) {
    tables_.emplace_back(tbl->deepCopy());
  }

  for (auto& table : tables_) {
    addChild(&table);
  }
}

GroupByMergeNode::GroupByMergeNode(
    Vector<RefPtr<QueryTreeNode>> tables) :
    tables_(tables) {
  for (auto& table : tables_) {
    addChild(&table);
  }
}

Vector<RefPtr<QueryTreeNode>> GroupByMergeNode::inputTables() const {
  return tables_;
}

Vector<String> GroupByMergeNode::outputColumns() const {
  if (tables_.empty()) {
    return Vector<String>{};
  } else {
    return tables_[0].asInstanceOf<TableExpressionNode>()->outputColumns();
  }
}

Vector<QualifiedColumn> GroupByMergeNode::allColumns() const {
  if (tables_.empty()) {
    return Vector<QualifiedColumn>{};
  } else {
    return tables_[0].asInstanceOf<TableExpressionNode>()->allColumns();
  }
}

size_t GroupByMergeNode::getColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  size_t idx = -1;

  for (auto& tbl : tables_) {
    auto tidx = tbl.asInstanceOf<TableExpressionNode>()->getColumnIndex(
        column_name,
        allow_add);

    if (idx != size_t(-1) && tidx != idx) {
      RAISEF(
          kRuntimeError,
          "column not found in GROUP BY tables: '$0'",
          column_name);
    }

    idx = tidx;
  }

  return idx;
}

Vector<TaskID> GroupByMergeNode::build(Transaction* txn, TaskDAG* tree) const {
  RAISE(kNotYetImplementedError, "not yet implemented");
}

RefPtr<QueryTreeNode> GroupByMergeNode::deepCopy() const {
  return new GroupByMergeNode(*this);
}

String GroupByMergeNode::toString() const {
  String str = "(group-merge";

  for (const auto& tbl : tables_) {
    str += " (subexpr " + tbl->toString() + ")";
  }

  str += ")";
  return str;
}

} // namespace csql
