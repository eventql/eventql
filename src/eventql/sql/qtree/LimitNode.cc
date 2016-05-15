/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#include <eventql/sql/qtree/LimitNode.h>
#include <eventql/sql/qtree/qtree_coder.h>

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

void LimitNode::encode(
    QueryTreeCoder* coder,
    const LimitNode& node,
    stx::OutputStream* os) {
  os->appendVarUInt(node.limit_);
  os->appendVarUInt(node.offset_);
  coder->encode(node.table_, os);
}

RefPtr<QueryTreeNode> LimitNode::decode(
    QueryTreeCoder* coder,
    stx::InputStream* is) {
  auto limit = is->readVarUInt();
  auto offset = is->readVarUInt();
  auto table = coder->decode(is);

  return new LimitNode(limit, offset, table);
}

} // namespace csql
