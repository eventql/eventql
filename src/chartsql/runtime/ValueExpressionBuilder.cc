/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <chartsql/parser/astnode.h>
#include <chartsql/parser/token.h>
#include <chartsql/runtime/compiler.h>
#include <chartsql/runtime/symboltable.h>
#include <chartsql/svalue.h>

namespace csql {

ValueExpressionBuilder::ValueExpressionBuilder(
    SymbolTable* symbol_table) :
    symbol_table_(symbol_table) {}

VM::Instruction* ValueExpressionBuilder::compile(ASTNode* ast, size_t* dynamic_storage_size) {
  RAISE(kNotImplementedError, "deprecated");
}

ScopedPtr<ValueExpression> ValueExpressionBuilder::compile(
    RefPtr<ValueExpressionNode> node) {
  ScratchMemory static_storage;
  size_t dynamic_storage_size = 0;

  auto expr = compileValueExpression(
      node,
      &dynamic_storage_size,
      &static_storage);

  return mkScoped(
      new ValueExpression(
          mkScoped(
              new VM::Program(
                  expr,
                  std::move(static_storage),
                  dynamic_storage_size))));
}

VM::Instruction* ValueExpressionBuilder::compileValueExpression(
   RefPtr<ValueExpressionNode> node,
   size_t* dynamic_storage_size,
   ScratchMemory* static_storage) {
  if (dynamic_cast<ColumnReferenceNode*>(node.get())) {
    return compileColumnReference(
        node.asInstanceOf<ColumnReferenceNode>(),
        static_storage);
  }

  if (dynamic_cast<LiteralExpressionNode*>(node.get())) {
    return compileLiteral(
        node.asInstanceOf<LiteralExpressionNode>(),
        dynamic_storage_size,
        static_storage);
  }

  if (dynamic_cast<IfExpressionNode*>(node.get())) {
    return compileIfStatement(
        node.asInstanceOf<IfExpressionNode>(),
        dynamic_storage_size,
        static_storage);
  }

  if (dynamic_cast<CallExpressionNode*>(node.get())) {
    return compileMethodCall(
        node.asInstanceOf<CallExpressionNode>(),
        dynamic_storage_size,
        static_storage);
  }

  RAISE(kRuntimeError, "internal error: can't compile expression");
}

VM::Instruction* ValueExpressionBuilder::compileLiteral(
    RefPtr<LiteralExpressionNode> node,
    size_t* dynamic_storage_size,
    ScratchMemory* static_storage) {
  auto ins = static_storage->construct<VM::Instruction>();
  ins->type = VM::X_LITERAL;
  ins->arg0 = static_storage->construct<SValue>(node->value());
  ins->child = nullptr;
  ins->next  = nullptr;

  return ins;
}

VM::Instruction* ValueExpressionBuilder::compileColumnReference(
    RefPtr<ColumnReferenceNode> node,
    ScratchMemory* static_storage) {
  auto col_idx = node->columnIndex();

  if (col_idx == size_t(-1)) {
    auto ins = static_storage->construct<VM::Instruction>();
    ins->type = VM::X_LITERAL;
    ins->arg0 = static_storage->construct<SValue>();
    ins->child = nullptr;
    ins->next  = nullptr;
    return ins;
  } else {
    auto ins = static_storage->construct<VM::Instruction>();
    ins->type = VM::X_INPUT;
    ins->arg0 = (void *) col_idx;
    ins->argn = 0;
    ins->child = nullptr;
    ins->next  = nullptr;
    return ins;
  }
}

VM::Instruction* ValueExpressionBuilder::compileMethodCall(
    RefPtr<CallExpressionNode> node,
    size_t* dynamic_storage_size,
    ScratchMemory* static_storage) {
  auto symbol = symbol_table_->lookup(node->symbol());
  const auto& args = node->arguments();

  auto op = static_storage->construct<VM::Instruction>();
  op->arg0  = nullptr;
  op->argn  = args.size();
  op->child = nullptr;
  op->next  = nullptr;

  switch (symbol.type) {
    case FN_PURE:
      op->type = VM::X_CALL_PURE;
      op->vtable.t_pure = symbol.vtable.t_pure;
      break;
    case FN_AGGREGATE:
      op->type = VM::X_CALL_AGGREGATE;
      op->arg0 = (void *) *dynamic_storage_size;
      op->vtable.t_aggregate = symbol.vtable.t_aggregate;
      *dynamic_storage_size += symbol.vtable.t_aggregate.scratch_size;
      break;
  }

  auto cur = &op->child;
  for (auto e : args) {
    auto next = compileValueExpression(
        e,
        dynamic_storage_size,
        static_storage);

    *cur = next;
    cur = &next->next;
  }

  return op;
}

VM::Instruction* ValueExpressionBuilder::compileIfStatement(
    RefPtr<IfExpressionNode> node,
    size_t* dynamic_storage_size,
    ScratchMemory* static_storage) {
  const auto& args = node->arguments();

  auto op = static_storage->construct<VM::Instruction>();
  op->type = VM::X_IF;
  op->arg0  = nullptr;
  op->argn  = args.size();
  op->child = nullptr;
  op->next  = nullptr;

  auto cur = &op->child;
  for (auto e : args) {
    auto next = compileValueExpression(
        e,
        dynamic_storage_size,
        static_storage);

    *cur = next;
    cur = &next->next;
  }

  return op;
}

}
