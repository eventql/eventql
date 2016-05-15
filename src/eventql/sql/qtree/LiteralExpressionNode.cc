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
#include <eventql/sql/qtree/LiteralExpressionNode.h>

using namespace stx;

namespace csql {

LiteralExpressionNode::LiteralExpressionNode(SValue value) : value_(value) {}

const SValue& LiteralExpressionNode::value() const {
  return value_;
}

Vector<RefPtr<ValueExpressionNode>> LiteralExpressionNode::arguments() const {
  return Vector<RefPtr<ValueExpressionNode>>{};
}

RefPtr<QueryTreeNode> LiteralExpressionNode::deepCopy() const {
  return new LiteralExpressionNode(value_);
}

String LiteralExpressionNode::toSQL() const {
  return value_.toSQL();
}

void LiteralExpressionNode::encode(
    QueryTreeCoder* coder,
    const LiteralExpressionNode& node,
    stx::OutputStream* os) {
  node.value().encode(os);
}

RefPtr<QueryTreeNode> LiteralExpressionNode::decode (
    QueryTreeCoder* coder,
    stx::InputStream* is) {
  SValue value;
  value.decode(is);
  return new LiteralExpressionNode(value);
}

} // namespace csql
