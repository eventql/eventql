// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#ifndef sw_flow_Flow_h
#define sw_flow_Flow_h

#include <cortex-flow/ASTVisitor.h>
#include <cortex-flow/Api.h>
#include <vector>

namespace cortex {
namespace flow {

//! \addtogroup Flow
//@{

class ASTNode;

class CORTEX_FLOW_API FlowCallVisitor : public ASTVisitor {
 private:
  std::vector<CallExpr*> calls_;

 public:
  explicit FlowCallVisitor(ASTNode* root = nullptr);
  ~FlowCallVisitor();

  void visit(ASTNode* root);

  void clear() { calls_.clear(); }

  const std::vector<CallExpr*>& calls() const { return calls_; }

 protected:
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
};

//!@}

}  // namespace flow
}  // namespace cortex

#endif
