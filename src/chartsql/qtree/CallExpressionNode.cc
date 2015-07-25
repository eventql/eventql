/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/CallExpressionNode.h>

using namespace stx;

namespace csql {

CallExpressionNode::CallExpressionNode(
    const String& symbol,
    Vector<RefPtr<ValueExpressionNode>> arguments) :
    symbol_(symbol),
    arguments_(arguments) {}

Vector<RefPtr<ValueExpressionNode>> CallExpressionNode::arguments() const {
  return arguments_;
}

const String& CallExpressionNode::symbol() const {
  return symbol_;
}

RefPtr<QueryTreeNode> CallExpressionNode::deepCopy() const {
  Vector<RefPtr<ValueExpressionNode>> args;
  for (const auto& arg : arguments_) {
    args.emplace_back(arg->deepCopyAs<ValueExpressionNode>());
  }

  return new CallExpressionNode(symbol_, args);
}

String CallExpressionNode::toSQL() const {
  Vector<String> args_sql;
  for (const auto& a : arguments_) {
    args_sql.emplace_back(a->toSQL());
  }

  return StringUtil::format(
      "$0($1)",
      symbol_,
      StringUtil::join(args_sql, ","));
}

} // namespace csql

