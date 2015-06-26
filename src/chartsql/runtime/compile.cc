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
#include <chartsql/runtime/compile.h>
#include <chartsql/runtime/symboltable.h>
#include <chartsql/svalue.h>

namespace csql {

Compiler::Compiler(SymbolTable* symbol_table) : symbol_table_(symbol_table) {}

Instruction* Compiler::compile(ASTNode* ast, size_t* scratchpad_size) {
  if (ast == nullptr) {
    RAISE(kNullPointerError, "can't compile nullptr");
  }

  switch (ast->getType()) {

    case ASTNode::T_SELECT_LIST:
      return compileSelectList(ast, scratchpad_size);

    case ASTNode::T_GROUP_BY:
      return compileChildren(ast, scratchpad_size);

    case ASTNode::T_EQ_EXPR:
      return compileOperator("eq", ast, scratchpad_size);

    case ASTNode::T_NEQ_EXPR:
      return compileOperator("neq", ast, scratchpad_size);

    case ASTNode::T_AND_EXPR:
      return compileOperator("and", ast, scratchpad_size);

    case ASTNode::T_OR_EXPR:
      return compileOperator("or", ast, scratchpad_size);

    case ASTNode::T_NEGATE_EXPR:
      return compileOperator("neg", ast, scratchpad_size);

    case ASTNode::T_LT_EXPR:
      return compileOperator("lt", ast, scratchpad_size);

    case ASTNode::T_LTE_EXPR:
      return compileOperator("lte", ast, scratchpad_size);

    case ASTNode::T_GT_EXPR:
      return compileOperator("gt", ast, scratchpad_size);

    case ASTNode::T_GTE_EXPR:
      return compileOperator("gte", ast, scratchpad_size);

    case ASTNode::T_ADD_EXPR:
      return compileOperator("add", ast, scratchpad_size);

    case ASTNode::T_SUB_EXPR:
      return compileOperator("sub", ast, scratchpad_size);

    case ASTNode::T_MUL_EXPR:
      return compileOperator("mul", ast, scratchpad_size);

    case ASTNode::T_DIV_EXPR:
      return compileOperator("div", ast, scratchpad_size);

    case ASTNode::T_MOD_EXPR:
      return compileOperator("mod", ast, scratchpad_size);

    case ASTNode::T_POW_EXPR:
      return compileOperator("pow", ast, scratchpad_size);

    case ASTNode::T_LITERAL:
      return compileLiteral(ast);

    case ASTNode::T_RESOLVED_COLUMN:
      return compileColumnReference(ast);

    case ASTNode::T_METHOD_CALL:
      return compileMethodCall(ast, scratchpad_size);

    default:
      ast->debugPrint();
      RAISE(kRuntimeError, "internal error: can't compile expression");
  }
}

ScopedPtr<CompiledProgram> Compiler::compile(
    RefPtr<ScalarExpressionNode> node) {
  size_t scratchpad_size = 0;
  auto expr = compileScalarExpression(node, &scratchpad_size);
  return mkScoped(new CompiledProgram(expr, scratchpad_size));
}

Instruction* Compiler::compileScalarExpression(
   RefPtr<ScalarExpressionNode> node,
   size_t* scratchpad_size) {

  if (dynamic_cast<FieldReferenceNode*>(node.get())) {
    return compileColumnReference(node.asInstanceOf<FieldReferenceNode>());
  }

  if (dynamic_cast<BuiltinExpressionNode*>(node.get())) {
    return compileMethodCall(
        node.asInstanceOf<BuiltinExpressionNode>(),
        scratchpad_size);
  }

  RAISE(kRuntimeError, "internal error: can't compile expression");
}

Instruction* Compiler::compileSelectList(
    ASTNode* select_list,
    size_t* scratchpad_size) {
  auto root = new Instruction();
  root->type = X_MULTI;
  root->call = nullptr;
  root->arg0 = nullptr;
  root->next  = nullptr;

  auto cur = &root->child;
  for (auto col : select_list->getChildren()) {
    if (!(*col == ASTNode::T_DERIVED_COLUMN)
        || col->getChildren().size() == 0) {
      RAISE(kRuntimeError, "internal error: corrupt ast");
    }

    auto next = compile(col->getChildren()[0], scratchpad_size);
    *cur = next;
    cur = &next->next;
  }

  return root;
}

Instruction* Compiler::compileChildren(
    ASTNode* parent,
    size_t* scratchpad_size) {
  auto root = new Instruction();
  root->type = X_MULTI;
  root->call = nullptr;
  root->arg0 = nullptr;
  root->next  = nullptr;

  auto cur = &root->child;
  for (auto child : parent->getChildren()) {
    auto next = compile(child, scratchpad_size);
    *cur = next;
    cur = &next->next;
  }

  return root;
}

Instruction* Compiler::compileOperator(
    const std::string& name,
    ASTNode* ast,
    size_t* scratchpad_size) {
  auto symbol = symbol_table_->lookupSymbol(name);

  if (symbol == nullptr) {
    RAISE(kRuntimeError, "undefined symbol: '%s'\n", name.c_str());
  }

  auto op = new Instruction();
  op->type = X_CALL;
  op->call = symbol->getFnPtr();
  op->arg0 = nullptr;
  op->child = nullptr;
  op->next  = nullptr;

  auto cur = &op->child;
  for (auto e : ast->getChildren()) {
    auto next = compile(e, scratchpad_size);
    *cur = next;
    cur = &next->next;
  }

  return op;
}

Instruction* Compiler::compileLiteral(ASTNode* ast) {
  if (ast->getToken() == nullptr) {
    RAISE(kRuntimeError, "internal error: corrupt ast");
  }

  auto ins = new Instruction();
  ins->type = X_LITERAL;
  ins->call = nullptr;
  ins->arg0 = SValue::fromToken(ast->getToken());
  ins->child = nullptr;
  ins->next  = nullptr;

  return ins;
}

Instruction* Compiler::compileColumnReference(ASTNode* ast) {
  auto ins = new Instruction();
  ins->type = X_INPUT;
  ins->call = nullptr;
  ins->arg0 = (void *) ast->getID();
  ins->child = nullptr;
  ins->next  = nullptr;
  return ins;
}

Instruction* Compiler::compileColumnReference(
    RefPtr<FieldReferenceNode> node) {
  auto ins = new Instruction();
  ins->type = X_INPUT;
  ins->call = nullptr;
  ins->arg0 = (void *) node->columnIndex();
  ins->argn = 0;
  ins->child = nullptr;
  ins->next  = nullptr;
  return ins;
}

Instruction* Compiler::compileMethodCall(
    ASTNode* ast,
    size_t* scratchpad_size) {
  if (ast->getToken() == nullptr ||
      ast->getToken()->getType() != Token::T_IDENTIFIER) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto symbol = symbol_table_->lookupSymbol(ast->getToken()->getString());
  if (symbol == nullptr) {
    RAISE(
        kRuntimeError,
        "error: cannot resolve symbol: %s\n",
        ast->getToken()->getString().c_str());
  }

  auto op = new Instruction();
  op->type = X_CALL;
  op->call = symbol->getFnPtr();
  op->arg0 = nullptr;
  op->child = nullptr;
  op->next  = nullptr;

  if (symbol->isAggregate()) {
    op->arg0 = (void *) *scratchpad_size;
    *scratchpad_size += symbol->getScratchpadSize();
  }

  auto cur = &op->child;
  for (auto e : ast->getChildren()) {
    auto next = compile(e, scratchpad_size);
    *cur = next;
    cur = &next->next;
  }

  return op;
}

Instruction* Compiler::compileMethodCall(
    RefPtr<BuiltinExpressionNode> node,
    size_t* scratchpad_size) {
  auto symbol = symbol_table_->lookup(node->symbol());
  const auto& args = node->arguments();

  auto op = new Instruction();
  op->arg0  = nullptr;
  op->argn  = args.size();
  op->child = nullptr;
  op->next  = nullptr;

  switch (symbol.type) {
    case FN_PURE:
      op->type = X_CALL_PURE;
      op->vtable.t_pure = symbol.vtable.t_pure;
      break;
    case FN_AGGREGATE:
      op->type = X_CALL_AGGREGATE;
      op->arg0 = (void *) *scratchpad_size;
      op->vtable.t_aggregate = symbol.vtable.t_aggregate;
      *scratchpad_size += symbol.vtable.t_aggregate.scratch_size;
      break;
  }

  auto cur = &op->child;
  for (auto e : args) {
    auto next = compileScalarExpression(e, scratchpad_size);
    *cur = next;
    cur = &next->next;
  }

  return op;

}

}
