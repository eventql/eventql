/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
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

using namespace stx;

namespace csql {
class ASTNode;

class Compiler {
public:

  static ScopedPtr<VM::Program> compile(
      Transaction* ctx,
      RefPtr<ValueExpressionNode> node,
      SymbolTable* symbol_table);

protected:

  static VM::Instruction* compileIfStatement(
      RefPtr<IfExpressionNode> node,
      size_t* dynamic_storage_size,
      ScratchMemory* static_storage,
      SymbolTable* symbol_table);

  static VM::Instruction* compileValueExpression(
      RefPtr<ValueExpressionNode> node,
      size_t* dynamic_storage_size,
      ScratchMemory* static_storage,
      SymbolTable* symbol_table);

  static VM::Instruction* compileLiteral(
      RefPtr<LiteralExpressionNode> node,
      size_t* dynamic_storage_size,
      ScratchMemory* static_storage,
      SymbolTable* symbol_table);

  static VM::Instruction* compileColumnReference(
      RefPtr<ColumnReferenceNode> node,
      ScratchMemory* static_storage,
      SymbolTable* symbol_table);

  static VM::Instruction* compileMethodCall(
      RefPtr<CallExpressionNode> node,
      size_t* dynamic_storage_size,
      ScratchMemory* static_storage,
      SymbolTable* symbol_table);

  static VM::Instruction* compileRegexOperator(
      RefPtr<RegexExpressionNode> node,
      size_t* dynamic_storage_size,
      ScratchMemory* static_storage,
      SymbolTable* symbol_table);

  static VM::Instruction* compileLikeOperator(
      RefPtr<LikeExpressionNode> node,
      size_t* dynamic_storage_size,
      ScratchMemory* static_storage,
      SymbolTable* symbol_table);


};

} // namespace csql
