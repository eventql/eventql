/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <eventql/sql/qtree/ColumnReferenceNode.h>

#include "eventql/eventql.h"

namespace csql {

ColumnReferenceNode::ColumnReferenceNode(
    const ColumnReferenceNode& other) :
    column_name_(other.column_name_),
    column_index_(other.column_index_),
    type_(other.type_) {}

ColumnReferenceNode::ColumnReferenceNode(
    const String& column_name,
    SType type) :
    column_name_(column_name),
    type_(type) {}

ColumnReferenceNode::ColumnReferenceNode(
    size_t column_index,
    SType type) :
    column_name_(StringUtil::toString(column_index)),
    column_index_(Some(column_index)),
    type_(type) {}

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
  if (column_name_.empty()) {
    return StringUtil::format("subquery_column($0)", column_index_.get());
  } else {
    return "`" + column_name_ + "`";
  }
}

SType ColumnReferenceNode::getReturnType() const {
  return type_;
}

void ColumnReferenceNode::encode(
    QueryTreeCoder* coder,
    const ColumnReferenceNode& node,
    OutputStream* os) {
  os->appendLenencString(node.column_name_);
  os->appendVarUInt((uint8_t) node.type_);

  if (!node.column_index_.isEmpty() && node.column_index_.get() != size_t(-1)) {
    os->appendUInt8(1);
    os->appendVarUInt(node.column_index_.get());
  } else {
    os->appendUInt8(0);
  }
}

RefPtr<QueryTreeNode> ColumnReferenceNode::decode (
    QueryTreeCoder* coder,
    InputStream* is) {
  auto column_name = is->readLenencString();
  auto type = (SType) is->readVarUInt();
  auto node = new ColumnReferenceNode(column_name, type);

  if (is->readUInt8()) {
    node->setColumnIndex(is->readVarUInt());
  }

  return node;
}

} // namespace csql
