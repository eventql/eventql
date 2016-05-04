/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/qtree/ColumnReferenceNode.h>

using namespace stx;

namespace csql {

ColumnReferenceNode::ColumnReferenceNode(
    const ColumnReferenceNode& other) :
    column_name_(other.column_name_),
    column_index_(other.column_index_) {}

ColumnReferenceNode::ColumnReferenceNode(
    const String& column_name) :
    column_name_(column_name) {}

ColumnReferenceNode::ColumnReferenceNode(
    size_t column_index) :
    column_name_(StringUtil::toString(column_index)),
    column_index_(Some(column_index)) {}

const String& ColumnReferenceNode::fieldName() const {
  return column_name_;
}

const String& ColumnReferenceNode::columnName() const {
  return column_name_;
}

void ColumnReferenceNode::setColumnName(const String& name) {
  column_name_ = name;
}

Vector<RefPtr<ValueExpressionNode>> ColumnReferenceNode::arguments() const {
  return Vector<RefPtr<ValueExpressionNode>>{};
}

size_t ColumnReferenceNode::columnIndex() const {
  if (column_index_.isEmpty()) {
    RAISE(
        kRuntimeError,
        "internal error: columnIndex called on a unresolved ColumnReference. "
        "did you try to execute an expression without resolving columns?");
  }

  return column_index_.get();
}

bool ColumnReferenceNode::hasColumnIndex() const {
  return !column_index_.isEmpty();
}

void ColumnReferenceNode::setColumnIndex(size_t index) {
  if (index == size_t(-1)) {
    return;
  }

  column_index_ = Some(index);
}

RefPtr<QueryTreeNode> ColumnReferenceNode::deepCopy() const {
  return new ColumnReferenceNode(*this);
}

String ColumnReferenceNode::toSQL() const {
  if (!column_index_.isEmpty() && column_index_.get() != size_t(-1)) {
    return StringUtil::format("subquery_column($0)", column_index_.get());
  }

  return "`" + column_name_ + "`";
}

} // namespace csql
