// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/Api.h>
#include <cortex-base/RegExp.h>
#include <cortex-base/net/IPAddress.h>
#include <cortex-base/net/Cidr.h>
#include <utility>
#include <memory>

namespace cortex {
namespace flow {

//! \addtogroup Flow
//@{

class Symbol;
class SymbolTable;
class ScopedSymbol;
class Variable;
class Handler;
class BuiltinFunction;
class BuiltinHandler;
class Unit;

class Expr;
class BinaryExpr;
class UnaryExpr;
template <typename>
class LiteralExpr;
class CallExpr;
class VariableExpr;
class HandlerRefExpr;
class ArrayExpr;

typedef LiteralExpr<std::string> StringExpr;
typedef LiteralExpr<long long> NumberExpr;
typedef LiteralExpr<bool> BoolExpr;
typedef LiteralExpr<RegExp> RegExpExpr;
typedef LiteralExpr<IPAddress> IPAddressExpr;
typedef LiteralExpr<Cidr> CidrExpr;

class Stmt;
class ExprStmt;
class CompoundStmt;
class CondStmt;
class MatchStmt;
class AssignStmt;

class CORTEX_FLOW_API ASTVisitor {
 public:
  virtual ~ASTVisitor() {}

  // symbols
  virtual void accept(Unit& symbol) = 0;
  virtual void accept(Variable& variable) = 0;
  virtual void accept(Handler& handler) = 0;
  virtual void accept(BuiltinFunction& symbol) = 0;
  virtual void accept(BuiltinHandler& symbol) = 0;

  // expressions
  virtual void accept(UnaryExpr& expr) = 0;
  virtual void accept(BinaryExpr& expr) = 0;
  virtual void accept(CallExpr& expr) = 0;
  virtual void accept(VariableExpr& expr) = 0;
  virtual void accept(HandlerRefExpr& expr) = 0;

  virtual void accept(StringExpr& expr) = 0;
  virtual void accept(NumberExpr& expr) = 0;
  virtual void accept(BoolExpr& expr) = 0;
  virtual void accept(RegExpExpr& expr) = 0;
  virtual void accept(IPAddressExpr& expr) = 0;
  virtual void accept(CidrExpr& array) = 0;
  virtual void accept(ArrayExpr& cidr) = 0;

  // statements
  virtual void accept(ExprStmt& stmt) = 0;
  virtual void accept(CompoundStmt& stmt) = 0;
  virtual void accept(CondStmt& stmt) = 0;
  virtual void accept(MatchStmt& stmt) = 0;
  virtual void accept(AssignStmt& stmt) = 0;
};

//!@}

}  // namespace flow
}  // namespace cortex
