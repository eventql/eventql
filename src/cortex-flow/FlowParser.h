// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#ifndef sw_flow_parser_h
#define sw_flow_parser_h (1)

#include <list>
#include <vector>
#include <string>
#include <functional>

#include <cortex-flow/Api.h>
#include <cortex-flow/FlowToken.h>
#include <cortex-flow/FlowLexer.h>
#include <cortex-flow/AST.h>  // SymbolTable
#include <cortex-base/Utility.h>

namespace cortex {
namespace flow {

//! \addtogroup Flow
//@{

namespace vm {
class Runtime;
class NativeCallback;
};

class FlowLexer;

class CORTEX_FLOW_API FlowParser {
  std::unique_ptr<FlowLexer> lexer_;
  SymbolTable* scopeStack_;
  vm::Runtime* runtime_;

 public:
  typedef std::function<void(const std::string&)> ErrorHandler;
  typedef std::function<bool(const std::string&, const std::string&,
                     std::vector<vm::NativeCallback*>*)> ImportHandler;

  ErrorHandler errorHandler;
  ImportHandler importHandler;

  explicit FlowParser(
      vm::Runtime* runtime,
      ImportHandler importHandler = nullptr,
      ErrorHandler errorHandler = nullptr);

  ~FlowParser();

  void openString(const std::string& filename);
  void openLocalFile(const std::string& filename);
  void openStream(std::unique_ptr<std::istream>&& ifs, const std::string& filename);

  std::unique_ptr<Unit> parse();

  vm::Runtime* runtime() const { return runtime_; }

 private:
  class Scope;

  // error handling
  void reportUnexpectedToken();
  void reportError(const std::string& message);
  template <typename... Args>
  void reportError(const std::string& fmt, Args&&...);

  // lexing
  FlowToken token() const { return lexer_->token(); }
  const FlowLocation& lastLocation() { return lexer_->lastLocation(); }
  const FlowLocation& location() { return lexer_->location(); }
  const FilePos& end() const { return lexer_->lastLocation().end; }
  FlowToken nextToken() const;
  bool eof() const { return lexer_->eof(); }
  bool expect(FlowToken token);
  bool consume(FlowToken);
  bool consumeIf(FlowToken);
  bool consumeUntil(FlowToken);

  template <typename A1, typename... Args>
  bool consumeOne(A1 token, Args... tokens);
  template <typename A1>
  bool testTokens(A1 token) const;
  template <typename A1, typename... Args>
  bool testTokens(A1 token, Args... tokens) const;

  std::string stringValue() const { return lexer_->stringValue(); }
  double numberValue() const { return lexer_->numberValue(); }
  bool booleanValue() const { return lexer_->numberValue(); }

  // scoping
  SymbolTable* scope() { return scopeStack_; }
  SymbolTable* globalScope();
  SymbolTable* enter(SymbolTable* scope);
  std::unique_ptr<SymbolTable> enterScope(const std::string& title) {
    return std::unique_ptr<SymbolTable>(enter(new SymbolTable(scope(), title)));
  }
  SymbolTable* leaveScope() { return leave(); }
  SymbolTable* leave();

  // symbol mgnt
  template <typename T>
  T* lookup(const std::string& name) {
    if (T* result = static_cast<T*>(scope()->lookup(name, Lookup::All)))
      return result;

    return nullptr;
  }

  template <typename T, typename... Args>
  T* createSymbol(Args&&... args) {
    return static_cast<T*>(scope()->appendSymbol(
        std::make_unique<T>(std::forward<Args>(args)...)));
  }

  template <typename T, typename... Args>
  T* lookupOrCreate(const std::string& name, Args&&... args) {
    if (T* result = static_cast<T*>(scope()->lookup(name, Lookup::All)))
      return result;

    // create symbol in global-scope
    T* result = new T(name, args...);
    scopeStack_->appendSymbol(result);
    return result;
  }

  void importRuntime();
  void declareBuiltin(const vm::NativeCallback* native);

  // syntax: decls
  std::unique_ptr<Unit> unit();
  bool importDecl(Unit* unit);
  bool importOne(std::list<std::string>& names);
  std::unique_ptr<Symbol> decl();
  std::unique_ptr<Variable> varDecl();
  std::unique_ptr<Handler> handlerDecl();

  // syntax: expressions
  std::unique_ptr<Expr> expr();
  std::unique_ptr<Expr> logicExpr();   // and or xor
  std::unique_ptr<Expr> notExpr();     // not
  std::unique_ptr<Expr> relExpr();     // == != <= >= < > =^ =$ =~
  std::unique_ptr<Expr> addExpr();     // + -
  std::unique_ptr<Expr> mulExpr();     // * / shl shr
  std::unique_ptr<Expr> powExpr();     // **
  std::unique_ptr<Expr> bitNotExpr();  // ~
  std::unique_ptr<Expr> negExpr();     // -
  std::unique_ptr<Expr> primaryExpr();
  std::unique_ptr<Expr> arrayExpr();
  std::unique_ptr<Expr> literalExpr();
  std::unique_ptr<Expr> interpolatedStr();
  std::unique_ptr<Expr> castExpr();

  std::unique_ptr<ParamList> paramList();
  std::unique_ptr<Expr> namedExpr(std::string* name);

  // syntax: statements
  std::unique_ptr<Stmt> stmt();
  std::unique_ptr<Stmt> ifStmt();
  std::unique_ptr<Stmt> matchStmt();
  std::unique_ptr<Stmt> compoundStmt();
  std::unique_ptr<Stmt> identStmt();
  std::unique_ptr<CallExpr> callStmt(const std::list<Symbol*>& callables);
  std::unique_ptr<CallExpr> resolve(const std::list<Callable*>& symbols,
                                    ParamList&& params);
  std::unique_ptr<Stmt> postscriptStmt(std::unique_ptr<Stmt> baseStmt);
};

// {{{ inlines
inline SymbolTable* FlowParser::globalScope() {
  if (SymbolTable* st = scopeStack_) {
    while (st->outerTable()) {
      st = st->outerTable();
    }
    return st;
  }
  return nullptr;
}

template <typename... Args>
inline void FlowParser::reportError(const std::string& fmt, Args&&... args) {
  char buf[1024];
  snprintf(buf, sizeof(buf), fmt.c_str(), args...);
  reportError(buf);
}

template <typename A1, typename... Args>
inline bool FlowParser::consumeOne(A1 a1, Args... tokens) {
  if (!testTokens(a1, tokens...)) {
    reportUnexpectedToken();
    return false;
  }

  nextToken();
  return true;
}

template <typename A1>
inline bool FlowParser::testTokens(A1 a1) const {
  return token() == a1;
}

template <typename A1, typename... Args>
inline bool FlowParser::testTokens(A1 a1, Args... tokens) const {
  if (token() == a1) return true;

  return testTokens(tokens...);
}

// }}}

//!@}

}  // namespace flow
}  // namespace cortex

#endif
