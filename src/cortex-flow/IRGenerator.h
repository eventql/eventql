// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/Api.h>
#include <cortex-flow/ASTVisitor.h>
#include <cortex-flow/ir/IRBuilder.h>
#include <deque>
#include <vector>
#include <string>

namespace cortex {
namespace flow {

//! \addtogroup Flow
//@{

/**
 * Transforms a Flow-AST into an SSA-conform IR.
 *
 */
class CORTEX_FLOW_API IRGenerator : public IRBuilder, public ASTVisitor {
 public:
  IRGenerator();
  ~IRGenerator();

  static std::unique_ptr<IRProgram> generate(
      Unit* unit, const std::vector<std::string>& exportedHandlers);

  void setErrorCallback(std::function<void(const std::string&)>&& handler) {
    onError_ = std::move(handler);
  }

  void setExports(const std::vector<std::string>& exports) {
    exports_ = exports;
  }

  std::unique_ptr<IRProgram> generate(Unit* unit);

 private:
  class Scope;

  std::vector<std::string> exports_;
  Scope* scope_;
  Value* result_;
  std::deque<Handler*> handlerStack_;

  size_t errorCount_;
  std::function<void(const std::string&)> onError_;

 private:
  Value* codegen(Expr* expr);
  Value* codegen(Stmt* stmt);
  Value* codegen(Symbol* sym);
  void codegenInline(Handler& handlerSym);

  Constant* getConstant(Expr* expr);

  Scope& scope() { return *scope_; }

  // symbols
  virtual void accept(Unit& symbol);
  virtual void accept(Variable& variable);
  virtual void accept(Handler& handler);
  virtual void accept(BuiltinFunction& symbol);
  virtual void accept(BuiltinHandler& symbol);

  // expressions
  virtual void accept(UnaryExpr& expr);
  virtual void accept(BinaryExpr& expr);
  virtual void accept(CallExpr& expr);
  virtual void accept(VariableExpr& expr);
  virtual void accept(HandlerRefExpr& expr);

  virtual void accept(StringExpr& expr);
  virtual void accept(NumberExpr& expr);
  virtual void accept(BoolExpr& expr);
  virtual void accept(RegExpExpr& expr);
  virtual void accept(IPAddressExpr& expr);
  virtual void accept(CidrExpr& cidr);
  virtual void accept(ArrayExpr& array);

  // statements
  virtual void accept(ExprStmt& stmt);
  virtual void accept(CompoundStmt& stmt);
  virtual void accept(CondStmt& stmt);
  virtual void accept(MatchStmt& stmt);
  virtual void accept(AssignStmt& stmt);

  // error handling
  void reportError(const std::string& message);
  template <typename... Args>
  void reportError(const std::string& fmt, Args&&...);
};

// {{{ class IRGenerator::Scope
class IRGenerator::Scope {
 private:
  std::unordered_map<Symbol*, Value*> scope_;

  std::unordered_map<Symbol*, std::list<Value*>> versions_;

 public:
  Scope() {}
  ~Scope() {}

  void clear() { scope_.clear(); }

  Value* lookup(Symbol* symbol) const {
    auto i = scope_.find(symbol);
    return i != scope_.end() ? i->second : nullptr;
  }

  void update(Symbol* symbol, Value* value) { scope_[symbol] = value; }

  void remove(Symbol* symbol) {
    auto i = scope_.find(symbol);
    if (i != scope_.end()) {
      scope_.erase(i);
    }
  }
};
// }}}

// {{{ IRGenerator inlines
template <typename... Args>
inline void IRGenerator::reportError(const std::string& fmt, Args&&... args) {
  char buf[1024];
  ssize_t n =
      snprintf(buf, sizeof(buf), fmt.c_str(), std::forward<Args>(args)...);

  if (n > 0) {
    reportError(buf);
  }
}
// }}}

//!@}

}  // namespace flow
}  // namespace cortex
