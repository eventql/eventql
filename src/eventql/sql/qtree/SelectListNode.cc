/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/qtree/SelectListNode.h>

using namespace stx;

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
    stx::OutputStream* os) {
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
    stx::InputStream* is) {
  auto node = new SelectListNode(
      coder->decode(is).asInstanceOf<ValueExpressionNode>());

  if (is->readUInt8()) {
    node->setAlias(is->readLenencString());
  }

  return node;
}


} // namespace csql
