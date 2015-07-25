// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/vm/Signature.h>
#include <cortex-flow/vm/Instruction.h>  // Opcode
#include <cortex-flow/vm/MatchClass.h>
#include <cortex-flow/FlowType.h>
#include <cortex-flow/FlowToken.h>
#include <cortex-flow/FlowLocation.h>
#include <cortex-flow/ASTVisitor.h>
#include <cortex-flow/Api.h>
#include <cortex-base/RegExp.h>
#include <utility>
#include <memory>
#include <vector>
#include <list>

namespace cortex {
namespace flow {

//! \addtogroup Flow
//@{

namespace vm {
class NativeCallback;
}

class ASTVisitor;
class FlowBackend;
class SymbolTable;
class Expr;

class CORTEX_FLOW_API ASTNode  // {{{
    {
 protected:
  FlowLocation location_;

 public:
  ASTNode() {}
  ASTNode(const FlowLocation& loc) : location_(loc) {}
  virtual ~ASTNode() {}

  FlowLocation& location() { return location_; }
  const FlowLocation& location() const { return location_; }
  void setLocation(const FlowLocation& loc) { location_ = loc; }

  virtual void visit(ASTVisitor& v) = 0;
};
// }}}
// {{{ Symbols
class CORTEX_FLOW_API Symbol : public ASTNode {
 public:
  enum Type { Variable = 1, Handler, BuiltinFunction, BuiltinHandler, Unit, };

 private:
  Type type_;
  std::string name_;

 protected:
  friend class SymbolTable;
  SymbolTable* owner_;

  Symbol(Type t, const std::string& name, const FlowLocation& loc)
      : ASTNode(loc), type_(t), name_(name), owner_(nullptr) {}

 public:
  Type type() const { return type_; }

  const std::string& name() const { return name_; }
  void setName(const std::string& value) { name_ = value; }
  SymbolTable* owner() const { return owner_; }
};

enum class Lookup {
  Self = 0x0001,          //!< local table only.
  Outer = 0x0002,         //!< outer scope.
  SelfAndOuter = 0x0003,  //!< local scope and any outer scopes
  All = 0xFFFF,
};

inline bool operator&(Lookup a, Lookup b) {
  return static_cast<unsigned>(a) & static_cast<unsigned>(b);
}

class CORTEX_FLOW_API SymbolTable {
 public:
  typedef std::vector<Symbol*> list_type;
  typedef list_type::iterator iterator;
  typedef list_type::const_iterator const_iterator;

 public:
  SymbolTable(SymbolTable* outer, const std::string& name);
  ~SymbolTable();

  // nested scoping
  void setOuterTable(SymbolTable* table) { outerTable_ = table; }
  SymbolTable* outerTable() const { return outerTable_; }

  // symbols
  Symbol* appendSymbol(std::unique_ptr<Symbol> symbol);
  void removeSymbol(Symbol* symbol);
  Symbol* symbolAt(size_t i) const;
  size_t symbolCount() const;

  Symbol* lookup(const std::string& name, Lookup lookupMethod) const;
  Symbol* lookup(const std::string& name, Lookup lookupMethod,
                 std::list<Symbol*>* result) const;

  template <typename T>
  T* lookup(const std::string& name, Lookup lookupMethod) {
    return dynamic_cast<T*>(lookup(name, lookupMethod));
  }

  iterator begin() { return symbols_.begin(); }
  iterator end() { return symbols_.end(); }

  const std::string& name() const { return name_; }

 private:
  list_type symbols_;
  SymbolTable* outerTable_;
  std::string name_;
};

class CORTEX_FLOW_API ScopedSymbol : public Symbol {
 protected:
  std::unique_ptr<SymbolTable> scope_;

 protected:
  ScopedSymbol(Type t, SymbolTable* outer, const std::string& name,
               const FlowLocation& loc)
      : Symbol(t, name, loc), scope_(new SymbolTable(outer, name)) {}

  ScopedSymbol(Type t, std::unique_ptr<SymbolTable>&& scope,
               const std::string& name, const FlowLocation& loc)
      : Symbol(t, name, loc), scope_(std::move(scope)) {}

 public:
  SymbolTable* scope() { return scope_.get(); }
  const SymbolTable* scope() const { return scope_.get(); }
  void setScope(std::unique_ptr<SymbolTable>&& table) {
    scope_ = std::move(table);
  }
};

class CORTEX_FLOW_API Variable : public Symbol {
 private:
  std::unique_ptr<Expr> initializer_;

 public:
  Variable(const std::string& name, std::unique_ptr<Expr>&& initializer,
           const FlowLocation& loc)
      : Symbol(Symbol::Variable, name, loc),
        initializer_(std::move(initializer)) {}

  Expr* initializer() const { return initializer_.get(); }
  void setInitializer(std::unique_ptr<Expr>&& value) {
    initializer_ = std::move(value);
  }

  // TODO: should we ref the scope here, for methods like these?
  //	bool isLocal() const { return !scope() || scope()->outerTable() !=
  //nullptr; }
  //	bool isGlobal() const { return !isLocal(); }

  virtual void visit(ASTVisitor& v);
};

class ParamList;

class CORTEX_FLOW_API Callable : public Symbol {
 protected:
  const vm::NativeCallback* nativeCallback_;
  vm::Signature sig_;

 public:
  Callable(Type t, const vm::NativeCallback* cb, const FlowLocation& loc);
  Callable(const std::string& name, const FlowLocation& loc);

  bool isHandler() const {
    return type() == Symbol::Handler || type() == Symbol::BuiltinHandler;
  }
  bool isFunction() const { return type() == Symbol::BuiltinFunction; }
  bool isBuiltin() const {
    return type() == Symbol::BuiltinHandler ||
           type() == Symbol::BuiltinFunction;
  }

  const vm::Signature& signature() const;

  const vm::NativeCallback* nativeCallback() const {
    return nativeCallback_;
  }

  /** checks whether given parameter list is a concrete match (without any
   * completions) to this symbol.
   */
  bool isDirectMatch(const ParamList& params) const;

  /** tries to match given parameters against this symbol by using default
   * values or reordering parameters (if named input args)
   */
  bool tryMatch(ParamList& params, Buffer* errorMessage) const;
};

class CORTEX_FLOW_API Handler : public Callable {
 private:
  std::unique_ptr<SymbolTable> scope_;
  std::unique_ptr<Stmt> body_;

 public:
  /** create forward-declared handler. */
  Handler(const std::string& name, const FlowLocation& loc)
      : Callable(name, loc), scope_(), body_(nullptr /*forward declared*/) {}

  /** create handler. */
  Handler(const std::string& name, std::unique_ptr<SymbolTable>&& scope,
          std::unique_ptr<Stmt>&& body, const FlowLocation& loc)
      : Callable(name, loc), scope_(std::move(scope)), body_(std::move(body)) {}

  SymbolTable* scope() { return scope_.get(); }
  const SymbolTable* scope() const { return scope_.get(); }
  Stmt* body() const { return body_.get(); }

  bool isForwardDeclared() const { return body_.get() == nullptr; }

  void implement(std::unique_ptr<SymbolTable>&& table,
                 std::unique_ptr<Stmt>&& body);

  virtual void visit(ASTVisitor& v);
};

class CORTEX_FLOW_API BuiltinFunction : public Callable {
 public:
  explicit BuiltinFunction(const vm::NativeCallback* cb)
      : Callable(Symbol::BuiltinFunction, cb, FlowLocation()) {}

  virtual void visit(ASTVisitor& v);
};

class CORTEX_FLOW_API BuiltinHandler : public Callable {
 public:
  explicit BuiltinHandler(const vm::NativeCallback* cb)
      : Callable(Symbol::BuiltinHandler, cb, FlowLocation()) {}

  virtual void visit(ASTVisitor& v);
};

class CORTEX_FLOW_API Unit : public ScopedSymbol {
 private:
  std::vector<std::pair<std::string, std::string>> modules_;

 public:
  Unit()
      : ScopedSymbol(Symbol::Unit, nullptr, "#unit", FlowLocation()),
        modules_() {}

  // plugins
  void import(const std::string& moduleName, const std::string& path) {
    modules_.push_back(std::make_pair(moduleName, path));
  }

  const std::vector<std::pair<std::string, std::string>>& modules() const {
    return modules_;
  }

  class Handler* findHandler(const std::string& name);

  virtual void visit(ASTVisitor& v);
};
// }}}
// {{{ Expr
class CORTEX_FLOW_API Expr : public ASTNode {
 protected:
  explicit Expr(const FlowLocation& loc) : ASTNode(loc) {}

 public:
  virtual FlowType getType() const = 0;
};

class CORTEX_FLOW_API UnaryExpr : public Expr {
 private:
  vm::Opcode operator_;
  std::unique_ptr<Expr> subExpr_;

 public:
  UnaryExpr(vm::Opcode op, std::unique_ptr<Expr>&& subExpr,
            const FlowLocation& loc)
      : Expr(loc), operator_(op), subExpr_(std::move(subExpr)) {}

  vm::Opcode op() const { return operator_; }
  Expr* subExpr() const { return subExpr_.get(); }

  virtual void visit(ASTVisitor& v);
  virtual FlowType getType() const;
};

class CORTEX_FLOW_API BinaryExpr : public Expr {
 private:
  vm::Opcode operator_;
  std::unique_ptr<Expr> lhs_;
  std::unique_ptr<Expr> rhs_;

 public:
  BinaryExpr(vm::Opcode op, std::unique_ptr<Expr>&& lhs,
             std::unique_ptr<Expr>&& rhs);

  vm::Opcode op() const { return operator_; }
  Expr* leftExpr() const { return lhs_.get(); }
  Expr* rightExpr() const { return rhs_.get(); }

  virtual void visit(ASTVisitor& v);
  virtual FlowType getType() const;
};

class CORTEX_FLOW_API ArrayExpr : public Expr {
 private:
  std::vector<std::unique_ptr<Expr>> values_;

 public:
  ArrayExpr(FlowLocation& loc, std::vector<std::unique_ptr<Expr>>&& values);
  ~ArrayExpr();

  const std::vector<std::unique_ptr<Expr>>& values() const { return values_; }
  std::vector<std::unique_ptr<Expr>>& values() { return values_; }

  virtual FlowType getType() const;

  virtual void visit(ASTVisitor& v);
};

template <typename T>
class CORTEX_FLOW_API LiteralExpr : public Expr {
 private:
  T value_;

 public:
  explicit LiteralExpr(const T& value) : Expr(FlowLocation()), value_(value) {}

  LiteralExpr(const T& value, const FlowLocation& loc)
      : Expr(loc), value_(value) {}

  const T& value() const { return value_; }
  void setValue(const T& value) { value_ = value; }

  virtual FlowType getType() const;

  virtual void visit(ASTVisitor& v) { v.accept(*this); }
};

class CORTEX_FLOW_API ParamList {
 private:
  bool isNamed_;
  std::vector<std::string> names_;
  std::vector<Expr*> values_;

 public:
  ParamList(const ParamList&) = delete;
  ParamList& operator=(const ParamList&) = delete;

  ParamList() : isNamed_(false), names_(), values_() {}
  explicit ParamList(bool named) : isNamed_(named), names_(), values_() {}
  ParamList(ParamList&& v)
      : isNamed_(v.isNamed_),
        names_(std::move(v.names_)),
        values_(std::move(v.values_)) {}
  ParamList& operator=(ParamList&& v);
  ~ParamList();

  void push_back(const std::string& name, std::unique_ptr<Expr>&& arg);
  void push_back(std::unique_ptr<Expr>&& arg);
  void replace(size_t index, std::unique_ptr<Expr>&& value);
  bool replace(const std::string& name, std::unique_ptr<Expr>&& value);

  bool contains(const std::string& name) const;
  void swap(size_t source, size_t dest);
  void reorder(const vm::NativeCallback* source,
               std::vector<std::string>* superfluous);
  int find(const std::string& name) const;

  bool isNamed() const { return isNamed_; }

  size_t size() const;
  bool empty() const;
  std::pair<std::string, Expr*> at(size_t offset) const;
  std::pair<std::string, Expr*> operator[](size_t offset) const {
    return at(offset);
  }
  const std::vector<std::string>& names() const { return names_; }
  const std::vector<Expr*> values() const { return values_; }

  Expr* front() const { return values_.front(); }
  Expr* back() const { return values_.back(); }

  void dump(const char* title = nullptr);

  FlowLocation location() const;
};

/**
 * Call to native function, native handler or source handler.
 *
 * @see Callable
 * @see ParamList
 */
class CORTEX_FLOW_API CallExpr : public Expr {
 private:
  Callable* callee_;
  ParamList args_;

 public:
  CallExpr(const FlowLocation& loc, Callable* callee, ParamList&& args)
      : Expr(loc), callee_(callee), args_(std::move(args)) {}

  Callable* callee() const { return callee_; }
  const ParamList& args() const { return args_; }
  ParamList& args() { return args_; }
  bool setArgs(ParamList&& args);

  virtual void visit(ASTVisitor& v);
  virtual FlowType getType() const;
};

class CORTEX_FLOW_API VariableExpr : public Expr {
 private:
  Variable* variable_;

 public:
  VariableExpr(Variable* var, const FlowLocation& loc)
      : Expr(loc), variable_(var) {}

  Variable* variable() const { return variable_; }
  void setVariable(Variable* var) { variable_ = var; }

  virtual void visit(ASTVisitor& v);
  virtual FlowType getType() const;
};

class CORTEX_FLOW_API HandlerRefExpr : public Expr {
 private:
  Handler* handler_;

 public:
  HandlerRefExpr(Handler* ref, const FlowLocation& loc)
      : Expr(loc), handler_(ref) {}

  Handler* handler() const { return handler_; }
  void setHandler(Handler* handler) { handler_ = handler; }

  virtual void visit(ASTVisitor& v);
  virtual FlowType getType() const;
};
// }}}
// {{{ Stmt
class CORTEX_FLOW_API Stmt : public ASTNode {
 protected:
  explicit Stmt(const FlowLocation& loc) : ASTNode(loc) {}
};

class CORTEX_FLOW_API ExprStmt : public Stmt {
 private:
  std::unique_ptr<Expr> expression_;

 public:
  explicit ExprStmt(std::unique_ptr<Expr>&& expr)
      : Stmt(expr->location()), expression_(std::move(expr)) {}

  Expr* expression() const { return expression_.get(); }
  void setExpression(std::unique_ptr<Expr>&& expr) {
    expression_ = std::move(expr);
  }

  virtual void visit(ASTVisitor&);
};

class CORTEX_FLOW_API CompoundStmt : public Stmt {
 private:
  std::list<std::unique_ptr<Stmt>> statements_;

 public:
  explicit CompoundStmt(const FlowLocation& loc) : Stmt(loc) {}

  void push_back(std::unique_ptr<Stmt>&& stmt);

  bool empty() const { return statements_.empty(); }
  size_t count() const { return statements_.size(); }

  std::list<std::unique_ptr<Stmt>>::iterator begin() {
    return statements_.begin();
  }
  std::list<std::unique_ptr<Stmt>>::iterator end() { return statements_.end(); }

  virtual void visit(ASTVisitor&);
};

class CORTEX_FLOW_API AssignStmt : public Stmt {
 private:
  Variable* variable_;
  std::unique_ptr<Expr> expr_;

 public:
  AssignStmt(Variable* var, std::unique_ptr<Expr> expr, const FlowLocation& loc)
      : Stmt(loc), variable_(var), expr_(std::move(expr)) {}

  Variable* variable() const { return variable_; }
  void setVariable(Variable* var) { variable_ = var; }

  Expr* expression() const { return expr_.get(); }
  void setExpression(std::unique_ptr<Expr> expr) { expr_ = std::move(expr); }

  virtual void visit(ASTVisitor&);
};

class CORTEX_FLOW_API CondStmt : public Stmt {
 private:
  std::unique_ptr<Expr> cond_;
  std::unique_ptr<Stmt> thenStmt_;
  std::unique_ptr<Stmt> elseStmt_;

 public:
  CondStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> thenStmt,
           std::unique_ptr<Stmt> elseStmt, const FlowLocation& loc)
      : Stmt(loc),
        cond_(std::move(cond)),
        thenStmt_(std::move(thenStmt)),
        elseStmt_(std::move(elseStmt)) {}

  Expr* condition() const { return cond_.get(); }
  void setCondition(std::unique_ptr<Expr> cond) { cond_ = std::move(cond); }

  Stmt* thenStmt() const { return thenStmt_.get(); }
  void setThenStmt(std::unique_ptr<Stmt> stmt) { thenStmt_ = std::move(stmt); }

  Stmt* elseStmt() const { return elseStmt_.get(); }
  void setElseStmt(std::unique_ptr<Stmt> stmt) { elseStmt_ = std::move(stmt); }

  virtual void visit(ASTVisitor&);
};

typedef std::pair<std::list<std::unique_ptr<Expr>>, std::unique_ptr<Stmt>>
MatchCase;

class CORTEX_FLOW_API MatchStmt : public Stmt {
 public:
  typedef std::list<MatchCase> CaseList;

  MatchStmt(const FlowLocation& loc, std::unique_ptr<Expr>&& cond,
            vm::MatchClass op, std::list<MatchCase>&& cases,
            std::unique_ptr<Stmt>&& elseStmt);
  MatchStmt(MatchStmt&& other);
  MatchStmt& operator=(MatchStmt&& other);
  ~MatchStmt();

  Expr* condition() { return cond_.get(); }
  vm::MatchClass op() const { return op_; }
  CaseList& cases() { return cases_; }

  Stmt* elseStmt() const { return elseStmt_.get(); }
  void setElseStmt(std::unique_ptr<Stmt> stmt) { elseStmt_ = std::move(stmt); }

  virtual void visit(ASTVisitor&);

 private:
  std::unique_ptr<Expr> cond_;
  vm::MatchClass op_;
  CaseList cases_;
  std::unique_ptr<Stmt> elseStmt_;
};
// }}}

//!@}

}  // namespace flow
}  // namespace cortex
