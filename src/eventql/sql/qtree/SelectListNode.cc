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
#include <eventql/sql/qtree/SelectListNode.h>

using namespace util;

namespace csql {

SelectListNode::SelectListNode(
    RefPtr<ValueExpressionNode> expr) :
    expr_(expr) {}

RefPtr<ValueExpressionNode> SelectListNode::expression() const {
  return expr_;
}

RefPtr<QueryTreeNode> SelectListNode::deepCopy() const {
  auto copy = mkRef(
      new SelectListNode(expr_->deepCopyAs<ValueExpressionNode>()));

  if (!alias_.isEmpty()) {
    copy->setAlias(alias_.get());
  }

  return copy.get();
}

String SelectListNode::columnName() const {
  if (!alias_.isEmpty()) {
    return alias_.get();
  }

  return expr_->toSQL();
}

void SelectListNode::setAlias(const String& alias) {
  alias_ = Some(alias);
}

String SelectListNode::toString() const {
  String str = "(select-list ";
  str += expr_->toString();

  if (!alias_.isEmpty()) {
    str += StringUtil::format(" (alias $0)", alias_.get());
  }

  str += ")";
  return str;
}

void SelectListNode::encode(
    QueryTreeCoder* coder,
    const SelectListNode& node,
    util::OutputStream* os) {
  coder->encode(node.expr_.get(), os);

  if (!node.alias_.isEmpty()) {
    os->appendUInt8(1);
    os->appendLenencString(node.alias_.get());
  } else {
    os->appendUInt8(0);
  }
}

RefPtr<QueryTreeNode> SelectListNode::decode(
    QueryTreeCoder* coder,
    util::InputStream* is) {
  auto node = new SelectListNode(
      coder->decode(is).asInstanceOf<ValueExpressionNode>());

  if (is->readUInt8()) {
    node->setAlias(is->readLenencString());
  }

  return node;
}


} // namespace csql
