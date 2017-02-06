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
#include "eventql/eventql.h"
#include <eventql/db/database.h>
#include <eventql/sql/qtree/CallExpressionNode.h>
#include <eventql/sql/runtime/symboltable.h>

namespace csql {

ReturnCode CallExpressionNode::newNode(
    Transaction* txn,
    const std::string& function_name,
    Vector<RefPtr<ValueExpressionNode>> arguments,
    RefPtr<ValueExpressionNode>* node) {
  std::vector<csql::SType> arg_types;
  for (const auto& arg : arguments) {
    arg_types.emplace_back(arg->getReturnType());
  }

  const SymbolTableEntry* symbol;
  auto rc = txn->getSymbolTable()->resolve(function_name, arg_types, &symbol);
  if (!rc.isSuccess()) {
    return rc;
  }

  return CallExpressionNode::newNode(function_name, symbol, arguments, node);
}

ReturnCode CallExpressionNode::newNode(
    const std::string& function_name,
    const SymbolTableEntry* symbol,
    Vector<RefPtr<ValueExpressionNode>> arguments,
    RefPtr<ValueExpressionNode>* node) {
  auto fun = symbol->getFunction();
  bool is_pure = fun->type == FN_PURE && !fun->has_side_effects;
  bool is_aggregate = fun->type == FN_AGGREGATE;

  *node = RefPtr<ValueExpressionNode>(
      new CallExpressionNode(
          function_name,
          symbol->getSymbol(),
          fun->return_type,
          is_pure,
          is_aggregate,
          arguments));

  return ReturnCode::success();
}

CallExpressionNode::CallExpressionNode(
    const String& function_name,
    const String& symbol,
    SType return_type,
    bool is_pure,
    bool is_aggregate,
    Vector<RefPtr<ValueExpressionNode>> arguments) :
    function_name_(function_name),
    symbol_(symbol),
    return_type_(return_type),
    is_pure_(is_pure),
    is_aggregate_(is_aggregate),
    arguments_(arguments) {}

Vector<RefPtr<ValueExpressionNode>> CallExpressionNode::arguments() const {
  return arguments_;
}

const String& CallExpressionNode::getFunctionName() const {
  return function_name_;
}

const String& CallExpressionNode::getSymbol() const {
  return symbol_;
}

bool CallExpressionNode::isPureFunction() const {
  return is_pure_;
}

bool CallExpressionNode::isAggregateFunction() const {
  return is_aggregate_;
}

RefPtr<QueryTreeNode> CallExpressionNode::deepCopy() const {
  Vector<RefPtr<ValueExpressionNode>> args;
  for (const auto& arg : arguments_) {
    args.emplace_back(arg->deepCopyAs<ValueExpressionNode>());
  }

  return new CallExpressionNode(
      function_name_,
      symbol_,
      return_type_,
      is_pure_,
      is_aggregate_,
      args);
}

String CallExpressionNode::toSQL() const {
  Vector<String> args_sql;
  for (const auto& a : arguments_) {
    args_sql.emplace_back(a->toSQL());
  }

  return StringUtil::format(
      "$0($1)",
      function_name_,
      StringUtil::join(args_sql, ","));
}

SType CallExpressionNode::getReturnType() const {
  return return_type_;
}

void CallExpressionNode::encode(
    QueryTreeCoder* coder,
    const CallExpressionNode& node,
    OutputStream* os) {
  os->appendLenencString(node.getFunctionName());
  os->appendLenencString(node.getSymbol());
  os->appendVarUInt((uint8_t) node.getReturnType());
  os->appendVarUInt(node.isPureFunction());
  os->appendVarUInt(node.isAggregateFunction());
  os->appendVarUInt(node.arguments_.size());
  for (auto arg : node.arguments_) {
    coder->encode(arg.get(), os);
  }
}

RefPtr<QueryTreeNode> CallExpressionNode::decode (
    QueryTreeCoder* coder,
    InputStream* is) {
  auto function_name = is->readLenencString();
  auto symbol = is->readLenencString();
  auto return_type = (SType) is->readVarUInt();
  auto is_pure = is->readVarUInt() > 0 ? true : false;
  auto is_aggregate = is->readVarUInt() > 0 ? true : false;

  Vector<RefPtr<ValueExpressionNode>> arguments;
  auto num_arguments = is->readVarUInt();
  for (size_t i = 0; i < num_arguments; ++i) {
    arguments.emplace_back(coder->decode(is).asInstanceOf<ValueExpressionNode>());
  }

  return new CallExpressionNode(
      function_name,
      symbol,
      return_type,
      is_pure,
      is_aggregate,
      arguments);
}

} // namespace csql

