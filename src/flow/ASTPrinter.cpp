// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/ASTPrinter.h>
#include <xzero/flow/AST.h>

namespace xzero {
namespace flow {

inline std::string escape(char value)  // {{{
{
  switch (value) {
    case '\t':
      return "<TAB>";
    case '\r':
      return "<CR>";
    case '\n':
      return "<LF>";
    case ' ':
      return "<SPACE>";
    default:
      break;
  }
  if (std::isprint(value)) {
    std::string s;
    s += value;
    return s;
  } else {
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%02X", value);
    return buf;
  }
}  // }}}
inline std::string escape(const std::string& value)  // {{{
{
  std::string result;

  for (const char ch : value) result += escape(ch);

  return result;
}  // }}}

void ASTPrinter::print(ASTNode* node) {
  ASTPrinter printer;
  node->visit(printer);
}

ASTPrinter::ASTPrinter() : depth_(0) {}

void ASTPrinter::prefix() {
  for (int i = 0; i < depth_; ++i) std::printf("  ");
}

void ASTPrinter::print(const char* title, ASTNode* node) {
  enter();
  if (node) {
    printf("%s\n", title);
    enter();
    node->visit(*this);
    leave();
  } else {
    printf("%s (nil)\n", title);
  }
  leave();
}

void ASTPrinter::print(const std::pair<std::string, Expr*>& node, size_t pos) {
  char buf[1024];
  if (!node.first.empty()) {
    snprintf(buf, sizeof(buf), "%s:", node.first.c_str());
  } else {
    snprintf(buf, sizeof(buf), "param #%zu:", pos);
  }
  print(buf, node.second);
}

void ASTPrinter::accept(Variable& variable) {
  if (variable.initializer()) {
    printf("Variable: %s as %s\n", variable.name().c_str(),
           tos(variable.initializer()->getType()).c_str());
    print("initializer", variable.initializer());
  } else {
    printf("Variable: %s (NULL)\n", variable.name().c_str());
  }
}

void ASTPrinter::accept(Handler& handler) {
  printf("Handler: %s\n", handler.name().c_str());

  enter();
  if (handler.isForwardDeclared()) {
    printf("handler is forward-declared (unresolved)\n");
  } else {
    printf("scope:\n");
    enter();
    for (Symbol* symbol : *handler.scope()) symbol->visit(*this);
    leave();

    printf("body:\n");
    enter();
    handler.body()->visit(*this);
    leave();
  }
  leave();
}

void ASTPrinter::accept(BuiltinFunction& symbol) {
  printf("BuiltinFunction: %s\n", symbol.signature().to_s().c_str());
}

void ASTPrinter::accept(BuiltinHandler& symbol) {
  printf("BuiltinHandler: %s\n", symbol.signature().to_s().c_str());
}

void ASTPrinter::accept(Unit& unit) {
  printf("Unit: %s\n", unit.name().c_str());

  enter();
  for (Symbol* symbol : *unit.scope()) symbol->visit(*this);

  leave();
}

void ASTPrinter::accept(UnaryExpr& expr) {
  printf("UnaryExpr: %s\n", mnemonic(expr.op()));
  print("subExpr", expr.subExpr());
}

void ASTPrinter::accept(BinaryExpr& expr) {
  printf("BinaryExpr: %s\n", mnemonic(expr.op()));
  enter();
  printf("lhs:\n");
  enter();
  expr.leftExpr()->visit(*this);
  leave();
  leave();
  enter();
  printf("rhs:\n");
  enter();
  expr.rightExpr()->visit(*this);
  leave();
  leave();
}

void ASTPrinter::accept(CallExpr& call) {
  printf("CallExpr: %s\n", call.callee()->signature().to_s().c_str());
  for (int i = 0, e = call.args().size(); i != e; ++i) {
    print(call.args()[i], i);
  }
}

void ASTPrinter::accept(VariableExpr& expr) {
  printf("VariableExpr: %s\n", expr.variable()->name().c_str());
}

void ASTPrinter::accept(HandlerRefExpr& handlerRef) {
  printf("HandlerRefExpr: %s\n", handlerRef.handler()->name().c_str());
}

void ASTPrinter::accept(StringExpr& string) {
  printf("StringExpr: \"%s\"\n", escape(string.value()).c_str());
}

void ASTPrinter::accept(NumberExpr& number) {
  printf("NumberExpr: %lli\n", number.value());
}

void ASTPrinter::accept(BoolExpr& boolean) {
  printf("BoolExpr: %s\n", boolean.value() ? "true" : "false");
}

void ASTPrinter::accept(RegExpExpr& regexp) {
  printf("RegExpExpr: /%s/\n", regexp.value().c_str());
}

void ASTPrinter::accept(IPAddressExpr& ipaddr) {
  printf("IPAddressExpr: %s\n", ipaddr.value().str().c_str());
}

void ASTPrinter::accept(CidrExpr& cidr) {
  printf("CidrExpr: %s\n", cidr.value().str().c_str());
}

void ASTPrinter::accept(ArrayExpr& array) {
  printf("ArrayExpr:\n");

  for (const auto& e : array.values()) {
    print("expr", e.get());
  }
}

void ASTPrinter::accept(ExprStmt& stmt) {
  printf("ExprStmt\n");
  print("expr", stmt.expression());
}

void ASTPrinter::accept(CompoundStmt& compoundStmt) {
  printf("CompoundStmt (%d statements)\n", compoundStmt.count());
  enter();
  for (auto& stmt : compoundStmt) {
    stmt->visit(*this);
  }
  leave();
}

void ASTPrinter::accept(CondStmt& cond) {
  printf("CondStmt\n");
  print("condition", cond.condition());
  print("thenStmt", cond.thenStmt());
  print("elseStmt", cond.elseStmt());
}

void ASTPrinter::accept(MatchStmt& match) {
  printf("MatchStmt: %s\n", tos(match.op()).c_str());
  print("cond", match.condition());
  for (auto& one : match.cases()) {
    printf("  case\n");
    enter();
    for (auto& label : one.first) {
      print("on", label.get());
    }
    print("stmt", one.second.get());
    leave();
  }
  print("else", match.elseStmt());
}

void ASTPrinter::accept(AssignStmt& assign) {
  printf("AssignStmt\n");
  enter();
  printf("lhs(var): %s\n", assign.variable()->name().c_str());
  leave();
  print("rhs", assign.expression());
}

}  // namespace flow
}  // namespace xzero
