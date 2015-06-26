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
#include <chartsql/qtree/ScalarExpressionNode.h>
#include <chartsql/qtree/FieldReferenceNode.h>
#include <chartsql/qtree/BuiltinExpressionNode.h>
#include <chartsql/runtime/symboltable.h>
#include <chartsql/runtime/ScalarExpression.h>
#include <chartsql/svalue.h>

using namespace fnord;

namespace csql {
class ASTNode;

class ScalarExpressionBuilder {
public:
  ScalarExpressionBuilder(SymbolTable* symbol_table);

  Instruction* compile(ASTNode* ast, size_t* scratchpad_size);

  ScopedPtr<ScalarExpression> compile(RefPtr<ScalarExpressionNode> node);

  SymbolTable* symbolTable() { return symbol_table_; }

protected:

  Instruction* compileScalarExpression(
     RefPtr<ScalarExpressionNode> node,
     size_t* scratchpad_size);

  Instruction* compileSelectList(
      ASTNode* select_list,
      size_t* scratchpad_size);

  Instruction* compileOperator(
      const std::string& name,
      ASTNode* ast,
      size_t* scratchpad_size);

  Instruction* compileLiteral(ASTNode* ast);

  Instruction* compileColumnReference(ASTNode* ast);

  Instruction* compileColumnReference(
      RefPtr<FieldReferenceNode> node);

  Instruction* compileChildren(ASTNode* ast, size_t* scratchpad_size);

  Instruction* compileMethodCall(ASTNode* ast, size_t* scratchpad_size);

  Instruction* compileMethodCall(
      RefPtr<BuiltinExpressionNode> node,
      size_t* scratchpad_size);

  SymbolTable* symbol_table_;
};
} // namespace csql
