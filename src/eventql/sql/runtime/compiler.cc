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
#include <assert.h>
#include <stdlib.h>
#include <eventql/util/RegExp.h>
#include <eventql/sql/parser/astnode.h>
#include <eventql/sql/parser/token.h>
#include <eventql/sql/runtime/compiler.h>
#include <eventql/sql/runtime/symboltable.h>
#include <eventql/sql/runtime/LikePattern.h>
#include <eventql/sql/svalue.h>

namespace csql {

ReturnCode Compiler::compile(
    Transaction* ctx,
    RefPtr<ValueExpressionNode> node,
    SymbolTable* symbol_table,
    std::unique_ptr<vm::Program>* output) {
  auto program = mkScoped(new vm::Program());
  program->return_type = node->getReturnType();
  program->method_call = vm::EntryPoint(0);

  /* compile root expression */
  auto rc = compileExpression(node, program.get(), symbol_table);
  if (!rc.isSuccess()) {
    return rc;
  }

  program->instructions.emplace_back(vm::X_RETURN, intptr_t(0));

  /* compile aggregate subexpression */
  // FIXME

  *output = std::move(program);
  return ReturnCode::success();
}

ReturnCode Compiler::compileExpression(
    RefPtr<ValueExpressionNode> node,
    vm::Program* program,
    SymbolTable* symbol_table) {
  if (dynamic_cast<ColumnReferenceNode*>(node.get())) {
    return compileColumnReference(
        node.asInstanceOf<ColumnReferenceNode>().get(),
        program,
        symbol_table);
  }

  //if (dynamic_cast<LiteralExpressionNode*>(node.get())) {
  //  return compileLiteral(
  //      node.asInstanceOf<LiteralExpressionNode>(),
  //      instance_storage_size,
  //      static_storage,
  //      symbol_table);
  //}

  if (dynamic_cast<IfExpressionNode*>(node.get())) {
    return compileIfExpression(
        node.asInstanceOf<IfExpressionNode>().get(),
        program,
        symbol_table);
  }

  if (dynamic_cast<CallExpressionNode*>(node.get())) {
    return compileMethodCall(
        node.asInstanceOf<CallExpressionNode>().get(),
        program,
        symbol_table);
  }

  return ReturnCode::error(
      "ERUNTIME",
      "internal error: can't compile expression");
}

ReturnCode Compiler::compileColumnReference(
    const ColumnReferenceNode* node,
    vm::Program* program,
    SymbolTable* symbol_table) {
  auto col_idx = node->columnIndex();
  assert(col_idx != size_t(-1));

  program->instructions.emplace_back(vm::X_INPUT, intptr_t(col_idx));

  return ReturnCode::success();
}

ReturnCode Compiler::compileIfExpression(
    const IfExpressionNode* node,
    vm::Program* program,
    SymbolTable* symbol_table) {
  {
    auto rc = compileExpression(node->conditional(), program, symbol_table);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  auto jump_idx = program->instructions.size();
  program->instructions.emplace_back(vm::X_CJUMP, intptr_t(0));

  {
    auto rc = compileExpression(node->falseBranch(), program, symbol_table);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  program->instructions[jump_idx].arg0 = program->instructions.size() + 1;
  program->instructions.emplace_back(vm::X_JUMP, intptr_t(0));
  jump_idx = program->instructions.size() - 1;

  {
    auto rc = compileExpression(node->trueBranch(), program, symbol_table);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  program->instructions[jump_idx].arg0 = program->instructions.size();

  return ReturnCode::success();
}

ReturnCode Compiler::compileMethodCall(
    const CallExpressionNode* node,
    vm::Program* program,
    SymbolTable* symbol_table) {
  auto entry = symbol_table->lookup(node->getSymbol());
  if (!entry) {
    return ReturnCode::errorf(
        "ERUNTIME",
        "symbol not found: $0",
        node->getSymbol());
  }

  auto fun = entry->getFunction();
  switch (fun->type) {

    case FN_PURE:
      for (auto e : node->arguments()) {
        auto rc = compileExpression(e, program, symbol_table);
        if (!rc.isSuccess()) {
          return rc;
        }
      }

      program->instructions.emplace_back(vm::X_CALL, intptr_t(fun->vtable.call));
      break;

    case FN_AGGREGATE:
      program->instructions.emplace_back(vm::X_CALL, intptr_t(fun->vtable.get));
      break;

  }

  return ReturnCode::success();
}

} // namespace csql

