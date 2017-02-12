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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/return_code.h>
#include <eventql/sql/qtree/ValueExpressionNode.h>
#include <eventql/sql/qtree/qtree_coder.h>
#include <eventql/sql/SFunction.h>
#include "eventql/eventql.h"

namespace csql {
class SymbolTableEntry;

class CallExpressionNode : public ValueExpressionNode {
public:

  static ReturnCode newNode (
      Transaction* txn,
      const std::string& function_name,
      Vector<RefPtr<ValueExpressionNode>> arguments,
      RefPtr<ValueExpressionNode>* node);

  static ReturnCode newNode (
      Transaction* txn,
      const std::string& function_name,
      const SymbolTableEntry* symbol,
      Vector<RefPtr<ValueExpressionNode>> arguments,
      RefPtr<ValueExpressionNode>* node);

  const String& getFunctionName() const;
  const String& getSymbol() const;
  SType getReturnType() const override;
  bool isPureFunction() const;
  bool isAggregateFunction() const;
  Vector<RefPtr<ValueExpressionNode>> arguments() const override;

  RefPtr<QueryTreeNode> deepCopy() const override;
  String toSQL() const override;

  static void encode(
      QueryTreeCoder* coder,
      const CallExpressionNode& node,
      OutputStream* os);

  static RefPtr<QueryTreeNode> decode (
      QueryTreeCoder* coder,
      InputStream* is);

protected:

  CallExpressionNode(
      const String& function_name,
      const String& symbol,
      SType return_type,
      bool is_pure,
      bool is_aggregate,
      Vector<RefPtr<ValueExpressionNode>> arguments);

  String function_name_;
  String symbol_;
  SType return_type_;
  bool is_pure_;
  bool is_aggregate_;
  Vector<RefPtr<ValueExpressionNode>> arguments_;
};

} // namespace csql
