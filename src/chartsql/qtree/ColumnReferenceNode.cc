/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/ColumnReferenceNode.h>

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

void ColumnReferenceNode::setColumnIndex(size_t index) {
  column_index_ = index;
}

RefPtr<QueryTreeNode> ColumnReferenceNode::deepCopy() const {
  return new ColumnReferenceNode(*this);
}

String ColumnReferenceNode::toSQL() const {
  return column_name_;
}

} // namespace csql
