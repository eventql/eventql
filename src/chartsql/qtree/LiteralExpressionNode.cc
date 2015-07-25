/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/LiteralExpressionNode.h>

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

} // namespace csql
