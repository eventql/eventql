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
#include <stx/stdtypes.h>
#include <csql/qtree/ValueExpressionNode.h>
#include <csql/qtree/ColumnReferenceNode.h>
#include <csql/qtree/CallExpressionNode.h>
#include <csql/qtree/LiteralExpressionNode.h>
#include <csql/qtree/IfExpressionNode.h>
#include <csql/qtree/RegexExpressionNode.h>
#include <csql/qtree/LikeExpressionNode.h>
#include <csql/runtime/symboltable.h>
#include <csql/runtime/ValueExpression.h>
#include <csql/runtime/vm.h>
#include <csql/runtime/ScratchMemory.h>
#include <csql/svalue.h>

using namespace stx;

namespace csql {
class ASTNode;

class Compiler {
public:

  static ScopedPtr<VM::Program> compile(
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
