/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/qtree/GroupByNode.h>
#include <eventql/sql/qtree/ColumnReferenceNode.h>

using namespace stx;

namespace csql {

GroupByNode::GroupByNode(
    Vector<RefPtr<SelectListNode>> select_list,
    Vector<RefPtr<ValueExpressionNode>> group_exprs,
    RefPtr<QueryTreeNode> table) :
    select_list_(select_list),
    group_exprs_(group_exprs),
    table_(table) {
  for (const auto& sl : select_list_) {
    column_names_.emplace_back(sl->columnName());
  }

  addChild(&table_);
}

GroupByNode::GroupByNode(
    const GroupByNode& other) :
    column_names_(other.column_names_),
    table_(other.table_->deepCopyAs<QueryTreeNode>()) {
  for (const auto& e : other.select_list_) {
    select_list_.emplace_back(e->deepCopyAs<SelectListNode>());
  }

  for (const auto& e : other.group_exprs_) {
    group_exprs_.emplace_back(e->deepCopyAs<ValueExpressionNode>());
  }

  addChild(&table_);
}

Vector<RefPtr<SelectListNode>> GroupByNode::selectList() const {
  return select_list_;
}

Vector<String> GroupByNode::outputColumns() const {
  return column_names_;
}

Vector<QualifiedColumn> GroupByNode::allColumns() const {
  return table_.asInstanceOf<TableExpressionNode>()->allColumns();
}

size_t GroupByNode::getColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  for (size_t i = 0; i < select_list_.size(); ++i) {
    if (select_list_[i]->columnName() == column_name) {
      return i;
    }
  }

  if (!allow_add) {
    return -1;
  }

  auto child_idx = table_
      .asInstanceOf<TableExpressionNode>()
      ->getColumnIndex(column_name, allow_add);

  if (child_idx != size_t(-1)) {
    auto slnode = new SelectListNode(new ColumnReferenceNode(child_idx));
    slnode->setAlias(column_name);
    select_list_.emplace_back(slnode);
    return select_list_.size() - 1;
  }

  return -1;
}

Vector<RefPtr<ValueExpressionNode>> GroupByNode::groupExpressions() const {
  return group_exprs_;
}

RefPtr<QueryTreeNode> GroupByNode::inputTable() const {
  return table_;
}

RefPtr<QueryTreeNode> GroupByNode::deepCopy() const {
  return new GroupByNode(*this);
}

String GroupByNode::toString() const {
  String str = "(group-by (select-list";

  for (const auto& e : select_list_) {
    str += " " + e->toString();
  }

  str += ") (group-list";
  for (const auto& e : group_exprs_) {
    str += " " + e->toString();
  }

  str += ") (subexpr " + table_->toString() + "))";

  return str;
}

void GroupByNode::encode(
    QueryTreeCoder* coder,
    const GroupByNode& node,
    stx::OutputStream* os) {
  os->appendVarUInt(node.select_list_.size());
  for (const auto& e : node.select_list_) {
    coder->encode(e.get(), os);
  }

  os->appendVarUInt(node.group_exprs_.size());
  for (const auto& e : node.group_exprs_) {
    coder->encode(e.get(), os);
  }

  coder->encode(node.table_, os);
}

RefPtr<QueryTreeNode> GroupByNode::decode (
    QueryTreeCoder* coder,
    stx::InputStream* is) {
  Vector<RefPtr<SelectListNode>> select_list;
  auto select_list_size = is->readVarUInt();
  for (auto i = 0; i < select_list_size; ++i) {
    select_list.emplace_back(coder->decode(is).asInstanceOf<SelectListNode>());
  }

  Vector<RefPtr<ValueExpressionNode>> group_exprs;
  auto group_exprs_size = is->readVarUInt();
  for (auto i = 0; i < group_exprs_size; ++i) {
    group_exprs.emplace_back(
        coder->decode(is).asInstanceOf<ValueExpressionNode>());
  }

  auto table = coder->decode(is);

  return new GroupByNode(select_list, group_exprs, table);
}


} // namespace csql
