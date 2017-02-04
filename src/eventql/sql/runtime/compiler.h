/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/sql/qtree/ValueExpressionNode.h>
#include <eventql/sql/qtree/ColumnReferenceNode.h>
#include <eventql/sql/qtree/CallExpressionNode.h>
#include <eventql/sql/qtree/LiteralExpressionNode.h>
#include <eventql/sql/qtree/IfExpressionNode.h>
#include <eventql/sql/qtree/RegexExpressionNode.h>
#include <eventql/sql/qtree/LikeExpressionNode.h>
#include <eventql/sql/runtime/symboltable.h>
#include <eventql/sql/runtime/ValueExpression.h>
#include <eventql/sql/runtime/vm.h>
#include <eventql/sql/runtime/ScratchMemory.h>
#include <eventql/sql/svalue.h>

#include "eventql/eventql.h"

namespace csql {
class ASTNode;

class Compiler {
public:

  static ScopedPtr<vm::Program> compile(
      Transaction* ctx,
      RefPtr<ValueExpressionNode> node,
      SymbolTable* symbol_table);

  static ReturnCode compile(
      Transaction* ctx,
      RefPtr<ValueExpressionNode> input,
      SymbolTable* symbol_table,
      std::unique_ptr<vm::Program>* output);

protected:

  static ReturnCode compileExpression(
      RefPtr<ValueExpressionNode> node,
      vm::Program* program,
      SymbolTable* symbol_table);

  static ReturnCode compileColumnReference(
      const ColumnReferenceNode* node,
      vm::Program* program,
      SymbolTable* symbol_table);

  static ReturnCode compileLiteral(
      const LiteralExpressionNode* node,
      vm::Program* program,
      SymbolTable* symbol_table);

  static ReturnCode compileIfExpression(
      const IfExpressionNode* node,
      vm::Program* program,
      SymbolTable* symbol_table);

  static ReturnCode compileMethodCall(
      const CallExpressionNode* node,
      vm::Program* program,
      SymbolTable* symbol_table);

};

} // namespace csql
