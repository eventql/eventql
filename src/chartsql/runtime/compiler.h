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
#include <chartsql/qtree/ValueExpressionNode.h>
#include <chartsql/qtree/ColumnReferenceNode.h>
#include <chartsql/qtree/CallExpressionNode.h>
#include <chartsql/qtree/LiteralExpressionNode.h>
#include <chartsql/qtree/IfExpressionNode.h>
#include <chartsql/runtime/symboltable.h>
#include <chartsql/runtime/ValueExpression.h>
#include <chartsql/runtime/vm.h>
#include <chartsql/runtime/ScratchMemory.h>
#include <chartsql/svalue.h>

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

};

} // namespace csql
