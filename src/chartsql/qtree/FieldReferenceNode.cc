/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/FieldReferenceNode.h>

using namespace fnord;

namespace csql {

FieldReferenceNode::FieldReferenceNode(
    const String& field_name) :
    field_name_(field_name) {}

const String& FieldReferenceNode::fieldName() const {
  return field_name_;
}

Vector<RefPtr<ScalarExpressionNode>> FieldReferenceNode::arguments() const {
  return Vector<RefPtr<ScalarExpressionNode>>{};
}

size_t FieldReferenceNode::columnIndex() const {
  if (column_index_.isEmpty()) {
    RAISE(
        kRuntimeError,
        "internal error: columnIndex called on a unresolved ColumnReference. "
        "did you try to execute an expression without resolving columns?");
  }

  return column_index_.get();
}

void FieldReferenceNode::setColumnIndex(size_t index) {
  column_index_ = index;
}

} // namespace csql
