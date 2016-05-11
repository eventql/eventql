/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/qtree/CallExpressionNode.h>

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

void CallExpressionNode::encode(
    QueryTreeCoder* coder,
    const CallExpressionNode& node,
    stx::OutputStream* os) {
  os->appendLenencString(node.symbol());
  os->appendUInt64(node.arguments_.size());
  for (auto arg : node.arguments_) {
    coder->encode(arg.get(), os);
  }
}

RefPtr<QueryTreeNode> CallExpressionNode::decode (
    QueryTreeCoder* coder,
    stx::InputStream* is) {
  auto symbol = is->readLenencString();

  Vector<RefPtr<ValueExpressionNode>> arguments;
  auto num_arguments = is->readUInt64();
  for (size_t i = 0; i < num_arguments; ++i) {
    arguments.emplace_back(coder->decode(is).asInstanceOf<ValueExpressionNode>());
  }

  return new CallExpressionNode(symbol, arguments);
}

} // namespace csql

