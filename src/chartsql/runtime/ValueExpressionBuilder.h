/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/stdtypes.h>
#include <chartsql/qtree/ValueExpressionNode.h>
#include <chartsql/qtree/ColumnReferenceNode.h>
#include <chartsql/qtree/CallExpressionNode.h>
#include <chartsql/qtree/LiteralExpressionNode.h>
#include <chartsql/qtree/IfExpressionNode.h>
#include <chartsql/runtime/symboltable.h>
#include <chartsql/runtime/ValueExpression.h>
#include <chartsql/svalue.h>

using namespace fnord;

namespace csql {
class ASTNode;

class ValueExpressionBuilder : public RefCounted{
public:

  ValueExpressionBuilder(SymbolTable* symbol_table);

  Instruction* compile(ASTNode* ast, size_t* dynamic_storage_size);

  ScopedPtr<ValueExpression> compile(RefPtr<ValueExpressionNode> node);

  SymbolTable* symbolTable() { return symbol_table_; }

protected:

  Instruction* compileIfStatement(
      RefPtr<IfExpressionNode> node,
      size_t* dynamic_storage_size,
      ScratchMemory* static_storage);

  Instruction* compileValueExpression(
      RefPtr<ValueExpressionNode> node,
      size_t* dynamic_storage_size,
      ScratchMemory* static_storage);

  Instruction* compileLiteral(
      RefPtr<LiteralExpressionNode> node,
      size_t* dynamic_storage_size,
      ScratchMemory* static_storage);

  Instruction* compileColumnReference(
      RefPtr<ColumnReferenceNode> node,
      ScratchMemory* static_storage);

  Instruction* compileMethodCall(
      RefPtr<CallExpressionNode> node,
      size_t* dynamic_storage_size,
      ScratchMemory* static_storage);

  SymbolTable* symbol_table_;
};
} // namespace csql
