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
#include <eventql/sql/qtree/CallExpressionNode.h>

#include "eventql/eventql.h"

namespace csql {

CallExpressionNode::CallExpressionNode(
    const String& symbol,
    SType return_type,
    Vector<RefPtr<ValueExpressionNode>> arguments) :
    symbol_(symbol),
    return_type_(return_type),
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

  return new CallExpressionNode(symbol_, return_type_, args);
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

SType CallExpressionNode::getReturnType() const {
  return return_type_;
}

void CallExpressionNode::encode(
    QueryTreeCoder* coder,
    const CallExpressionNode& node,
    OutputStream* os) {
  os->appendLenencString(node.symbol());
  os->appendVarUInt((uint8_t) node.getReturnType());
  os->appendVarUInt(node.arguments_.size());
  for (auto arg : node.arguments_) {
    coder->encode(arg.get(), os);
  }
}

RefPtr<QueryTreeNode> CallExpressionNode::decode (
    QueryTreeCoder* coder,
    InputStream* is) {
  auto symbol = is->readLenencString();
  auto return_type = (SType) is->readVarUInt();;

  Vector<RefPtr<ValueExpressionNode>> arguments;
  auto num_arguments = is->readVarUInt();
  for (size_t i = 0; i < num_arguments; ++i) {
    arguments.emplace_back(coder->decode(is).asInstanceOf<ValueExpressionNode>());
  }

  return new CallExpressionNode(symbol, return_type, arguments);
}

} // namespace csql

