// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/vm/NativeCallback.h>
#include <xzero/flow/FlowParser.h>
#include <xzero/flow/FlowLexer.h>
#include <xzero/flow/AST.h>
#include <xzero/flow/vm/Runtime.h>
#include <xzero/Utility.h>
#include <unordered_map>
#include <unistd.h>

enum class OpSig {
  Invalid,
  BoolBool,
  NumNum,
  StringString,
  StringRegexp,
  IpIp,
  IpCidr,
  CidrCidr,
};

namespace std {
template <>
struct hash<OpSig> {
  uint32_t operator()(OpSig v) const noexcept {
    return static_cast<uint32_t>(v);
  }
};
}

namespace xzero {
namespace flow {

using vm::Opcode;

//#define FLOW_DEBUG_PARSER 1

#if defined(FLOW_DEBUG_PARSER)
// {{{ trace
static size_t fnd = 0;
struct fntrace {
  std::string msg_;

  fntrace(const char* msg) : msg_(msg) {
    size_t i = 0;
    char fmt[1024];

    for (i = 0; i < 2 * fnd;) {
      fmt[i++] = ' ';
      fmt[i++] = ' ';
    }
    fmt[i++] = '-';
    fmt[i++] = '>';
    fmt[i++] = ' ';
    strcpy(fmt + i, msg_.c_str());

    XZERO_DEBUG("FlowParser", 5, "%s", fmt);
    ++fnd;
  }

  ~fntrace() {
    --fnd;

    size_t i = 0;
    char fmt[1024];

    for (i = 0; i < 2 * fnd;) {
      fmt[i++] = ' ';
      fmt[i++] = ' ';
    }
    fmt[i++] = '<';
    fmt[i++] = '-';
    fmt[i++] = ' ';
    strcpy(fmt + i, msg_.c_str());

    XZERO_DEBUG("FlowParser", 5, "%s", fmt);
  }
};
// }}}
#define FNTRACE() fntrace _(__PRETTY_FUNCTION__)
#define TRACE(level, msg...) XZERO_DEBUG("FlowParser", (level), msg)
#else
#define FNTRACE()            do {} while (0)
#define TRACE(level, msg...) do {} while (0)
#endif

// {{{ scoped(SCOPED_SYMBOL)
class FlowParser::Scope {
 private:
  FlowParser* parser_;
  SymbolTable* table_;
  bool flipped_;

 public:
  Scope(FlowParser* parser, SymbolTable* table)
      : parser_(parser), table_(table), flipped_(false) {
    parser_->enter(table_);
  }

  Scope(FlowParser* parser, ScopedSymbol* symbol)
      : parser_(parser), table_(symbol->scope()), flipped_(false) {
    parser_->enter(table_);
  }

  Scope(FlowParser* parser, std::unique_ptr<SymbolTable>& table)
      : parser_(parser), table_(table.get()), flipped_(false) {
    parser_->enter(table_);
  }

  template <typename T>
  Scope(FlowParser* parser, std::unique_ptr<T>& symbol)
      : parser_(parser), table_(symbol->scope()), flipped_(false) {
    parser_->enter(table_);
  }

  bool flip() {
    flipped_ = !flipped_;
    return flipped_;
  }

  ~Scope() { parser_->leave(); }
};
#define scoped(SCOPED_SYMBOL) \
  for (FlowParser::Scope _(this, (SCOPED_SYMBOL)); _.flip();)
// }}}

FlowParser::FlowParser(vm::Runtime* runtime)
    : lexer_(new FlowLexer()),
      scopeStack_(nullptr),
      runtime_(runtime),
      errorHandler(),
      importHandler() {
  // enter(new SymbolTable(nullptr, "global"));
}

FlowParser::~FlowParser() {
  // leave();
  // assert(scopeStack_ == nullptr && "scopeStack not properly unwind. probably
  // a bug.");
}

bool FlowParser::open(const std::string& filename) {
  return lexer_->open(filename);
}

bool FlowParser::open(const std::string& filename,
                      std::unique_ptr<std::istream>&& ifs) {
  return lexer_->open(filename, std::move(ifs));
}

SymbolTable* FlowParser::enter(SymbolTable* scope) {
  // printf("Parser::enter(): new top: %p \"%s\" (outer: %p \"%s\")\n", scope,
  // scope->name().c_str(), scopeStack_, scopeStack_ ?
  // scopeStack_->name().c_str() : "");

  scope->setOuterTable(scopeStack_);
  scopeStack_ = scope;

  return scope;
}

SymbolTable* FlowParser::leave() {
  SymbolTable* popped = scopeStack_;
  scopeStack_ = scopeStack_->outerTable();

  // printf("Parser::leave(): new top: %p \"%s\" (outer: %p \"%s\")\n", popped,
  // popped->name().c_str(), scopeStack_, scopeStack_ ?
  // scopeStack_->name().c_str() : "");

  return popped;
}

std::unique_ptr<Unit> FlowParser::parse() { return unit(); }

// {{{ type system
/**
 * Computes VM opcode for binary operation.
 *
 * @param token operator between left and right expression
 * @param left left hand expression
 * @param right right hand expression
 *
 * @return opcode that matches given expressions operator or EXIT if operands
 *incompatible.
 */
vm::Opcode makeOperator(FlowToken token, Expr* left, Expr* right) {
  // (bool, bool)     == !=
  // (num, num)       + - * / % ** << >> & | ^ and or xor == != <= >= < >
  // (string, string) + == != <= >= < > =^ =$ in
  // (string, regex)  =~
  // (ip, ip)         == !=
  // (ip, cidr)       in
  // (cidr, cidr)     == != in

  auto isString = [](FlowType t) { return t == FlowType::String; };
  auto isNumber = [](FlowType t) { return t == FlowType::Number; };
  auto isBool = [](FlowType t) { return t == FlowType::Boolean; };
  auto isIPAddr = [](FlowType t) { return t == FlowType::IPAddress; };
  auto isCidr = [](FlowType t) { return t == FlowType::Cidr; };
  auto isRegExp = [](FlowType t) { return t == FlowType::RegExp; };

  FlowType leftType = left->getType();
  FlowType rightType = right->getType();

  OpSig opsig = OpSig::Invalid;
  if (isBool(leftType) && isBool(rightType))
    opsig = OpSig::BoolBool;
  else if (isNumber(leftType) && isNumber(rightType))
    opsig = OpSig::NumNum;
  else if (isString(leftType) && isString(rightType))
    opsig = OpSig::StringString;
  else if (isString(leftType) && isRegExp(rightType))
    opsig = OpSig::StringRegexp;
  else if (isIPAddr(leftType) && isIPAddr(rightType))
    opsig = OpSig::IpIp;
  else if (isIPAddr(leftType) && isCidr(rightType))
    opsig = OpSig::IpCidr;
  else if (isCidr(leftType) && isCidr(rightType))
    opsig = OpSig::CidrCidr;

  static const std::unordered_map<OpSig, std::unordered_map<FlowToken, Opcode>>
  ops = {{OpSig::BoolBool,
          {{FlowToken::Equal, Opcode::NCMPEQ},
           {FlowToken::UnEqual, Opcode::NCMPNE},
           {FlowToken::And, Opcode::BAND},
           {FlowToken::Or, Opcode::BOR},
           {FlowToken::Xor, Opcode::BXOR}, }},
         {OpSig::NumNum,
          {{FlowToken::Plus, Opcode::NADD},
           {FlowToken::Minus, Opcode::NSUB},
           {FlowToken::Mul, Opcode::NMUL},
           {FlowToken::Div, Opcode::NDIV},
           {FlowToken::Mod, Opcode::NREM},
           {FlowToken::Pow, Opcode::NPOW},
           {FlowToken::Shl, Opcode::NSHL},
           {FlowToken::Shr, Opcode::NSHR},
           {FlowToken::BitAnd, Opcode::NAND},
           {FlowToken::BitOr, Opcode::NOR},
           {FlowToken::BitXor, Opcode::NXOR},
           {FlowToken::Equal, Opcode::NCMPEQ},
           {FlowToken::UnEqual, Opcode::NCMPNE},
           {FlowToken::LessOrEqual, Opcode::NCMPLT},
           {FlowToken::GreaterOrEqual, Opcode::NCMPGT},
           {FlowToken::Less, Opcode::NCMPLE},
           {FlowToken::Greater, Opcode::NCMPGE}, }},
         {OpSig::StringString,
          {{FlowToken::Plus, Opcode::SADD},
           {FlowToken::Equal, Opcode::SCMPEQ},
           {FlowToken::UnEqual, Opcode::SCMPNE},
           {FlowToken::LessOrEqual, Opcode::SCMPLE},
           {FlowToken::GreaterOrEqual, Opcode::SCMPGE},
           {FlowToken::Less, Opcode::SCMPLT},
           {FlowToken::Greater, Opcode::SCMPGT},
           {FlowToken::PrefixMatch, Opcode::SCMPBEG},
           {FlowToken::SuffixMatch, Opcode::SCMPEND},
           {FlowToken::In, Opcode::SCONTAINS}, }},
         {OpSig::StringRegexp, {{FlowToken::RegexMatch, Opcode::SREGMATCH}, }},
         {OpSig::IpIp,
          {{FlowToken::Equal, Opcode::PCMPEQ},
           {FlowToken::UnEqual, Opcode::PCMPNE}, }},
         {OpSig::IpCidr, {{FlowToken::In, Opcode::PINCIDR}, }},
         {OpSig::CidrCidr,
          {{FlowToken::Equal, Opcode::NOP},    // TODO
           {FlowToken::UnEqual, Opcode::NOP},  // TODO
           {FlowToken::In, Opcode::NOP},       // TODO
          }}, };

  auto a = ops.find(opsig);
  if (a == ops.end()) return Opcode::EXIT;

  auto b = a->second.find(token);
  if (b == a->second.end()) return Opcode::EXIT;

  return b->second;
}

Opcode makeOperator(FlowToken token, Expr* e) {
  static const std::unordered_map<FlowType,
                                  std::unordered_map<FlowToken, Opcode>> ops =
      {{FlowType::Number,
        {{FlowToken::Not, Opcode::NCMPZ},
         {FlowToken::BitNot, Opcode::NNOT},
         {FlowToken::Minus, Opcode::NNEG},
         {FlowToken::StringType, Opcode::I2S},
         {FlowToken::BoolType, Opcode::NCMPZ},
         {FlowToken::NumberType, Opcode::NOP}, }},
       {FlowType::Boolean,
        {{FlowToken::Not, Opcode::BNOT},
         {FlowToken::BoolType, Opcode::NOP},
         {FlowToken::StringType,
          Opcode::I2S},  // XXX or better print "true" | "false" ?
        }},
       {FlowType::String,
        {{FlowToken::Not, Opcode::SISEMPTY},
         {FlowToken::NumberType, Opcode::S2I},
         {FlowToken::StringType, Opcode::NOP}, }},
       {FlowType::IPAddress, {{FlowToken::StringType, Opcode::P2S}, }},
       {FlowType::Cidr, {{FlowToken::StringType, Opcode::C2S}, }},
       {FlowType::RegExp, {{FlowToken::StringType, Opcode::R2S}, }}, };

  FlowType type = e->getType();

  auto a = ops.find(type);
  if (a == ops.end()) return Opcode::EXIT;

  auto b = a->second.find(token);
  if (b == a->second.end()) return Opcode::EXIT;

  return b->second;
}
// }}}
// {{{ error mgnt
void FlowParser::reportUnexpectedToken() {
  reportError("Unexpected token '%s'", token().c_str());
}

void FlowParser::reportError(const std::string& message) {

  if (!errorHandler) {
    char buf[1024];
    int n = snprintf(buf, sizeof(buf), "[%04zu:%02zu] %s\n", lexer_->line(),
                     lexer_->column(), message.c_str());
    write(2, buf, n);
    return;
  }

  char buf[1024];
  snprintf(buf, sizeof(buf), "[%04zu:%02zu] %s", lexer_->line(),
           lexer_->column(), message.c_str());
  errorHandler(buf);
}
// }}}
// {{{ lexing
FlowToken FlowParser::nextToken() const { return lexer_->nextToken(); }

bool FlowParser::expect(FlowToken value) {
  if (token() != value) {
    reportError("Unexpected token '%s' (expected: '%s')", token().c_str(),
                value.c_str());
    return false;
  }
  return true;
}

bool FlowParser::consume(FlowToken value) {
  if (!expect(value)) return false;

  nextToken();
  return true;
}

bool FlowParser::consumeIf(FlowToken value) {
  if (token() == value) {
    nextToken();
    return true;
  } else {
    return false;
  }
}

bool FlowParser::consumeUntil(FlowToken value) {
  for (;;) {
    if (token() == value) {
      nextToken();
      return true;
    }

    if (token() == FlowToken::Eof) return false;

    nextToken();
  }
}
// }}}
// {{{ decls
std::unique_ptr<Unit> FlowParser::unit() {
  auto unit = std::make_unique<Unit>();

  scoped(unit) {
    importRuntime();

    while (token() == FlowToken::Import) {
      if (!importDecl(unit.get())) {
        return nullptr;
      }
    }

    while (std::unique_ptr<Symbol> symbol = decl()) {
      scope()->appendSymbol(std::move(symbol));
    }
  }

  return unit;
}

void FlowParser::importRuntime() {
  if (runtime_) {
    TRACE(1, "importing runtime, %zu builtins", runtime_->builtins().size());

    for (const auto& builtin : runtime_->builtins()) {
      declareBuiltin(builtin);
    }
  }
}

void FlowParser::declareBuiltin(const vm::NativeCallback* native) {
  TRACE(1, "declareBuiltin (scope:%p): %s", scope(),
        native->signature().to_s().c_str());

  if (native->isHandler()) {
    createSymbol<BuiltinHandler>(native);
  } else {
    createSymbol<BuiltinFunction>(native);
  }
}

std::unique_ptr<Symbol> FlowParser::decl() {
  FNTRACE();

  switch (token()) {
    case FlowToken::Var:
      return varDecl();
    case FlowToken::Handler:
      return handlerDecl();
    default:
      return nullptr;
  }
}

// 'var' IDENT ['=' EXPR] ';'
std::unique_ptr<Variable> FlowParser::varDecl() {
  FNTRACE();
  FlowLocation loc(lexer_->location());

  if (!consume(FlowToken::Var)) return nullptr;

  if (!consume(FlowToken::Ident)) return nullptr;

  std::string name = stringValue();

  if (!consume(FlowToken::Assign)) return nullptr;

  std::unique_ptr<Expr> initializer = expr();
  if (!initializer) return nullptr;

  loc.update(initializer->location().end);
  consume(FlowToken::Semicolon);

  return std::make_unique<Variable>(name, std::move(initializer), loc);
}

bool FlowParser::importDecl(Unit* unit) {
  FNTRACE();

  // 'import' NAME_OR_NAMELIST ['from' PATH] ';'
  nextToken();  // skip 'import'

  std::list<std::string> names;
  if (!importOne(names)) {
    consumeUntil(FlowToken::Semicolon);
    return false;
  }

  while (token() == FlowToken::Comma) {
    nextToken();
    if (!importOne(names)) {
      consumeUntil(FlowToken::Semicolon);
      return false;
    }
  }

  std::string path;
  if (consumeIf(FlowToken::From)) {
    path = stringValue();

    if (!consumeOne(FlowToken::String, FlowToken::RawString)) {
      consumeUntil(FlowToken::Semicolon);
      return false;
    }

    if (!path.empty() && path[0] != '/') {
      std::string base(lexer_->location().filename);

      size_t r = base.rfind('/');
      if (r != std::string::npos) ++r;

      base = base.substr(0, r);
      path = base + path;
    }
  }

  for (auto i = names.begin(), e = names.end(); i != e; ++i) {
    std::vector<vm::NativeCallback*> builtins;

    if (importHandler && !importHandler(*i, path, &builtins)) return false;

    unit->import(*i, path);

    for (vm::NativeCallback* native : builtins) {
      declareBuiltin(native);
    }
  }

  consume(FlowToken::Semicolon);
  return true;
}

bool FlowParser::importOne(std::list<std::string>& names) {
  switch (token()) {
    case FlowToken::Ident:
    case FlowToken::String:
    case FlowToken::RawString:
      names.push_back(stringValue());
      nextToken();
      break;
    case FlowToken::RndOpen:
      nextToken();
      if (!importOne(names)) return false;

      while (token() == FlowToken::Comma) {
        nextToken();  // skip comma
        if (!importOne(names)) return false;
      }

      if (!consume(FlowToken::RndClose)) return false;
      break;
    default:
      reportError("Syntax error in import declaration.");
      return false;
  }
  return true;
}

// handlerDecl ::= 'handler' IDENT (';' | [do] stmt)
std::unique_ptr<Handler> FlowParser::handlerDecl() {
  FNTRACE();

  FlowLocation loc(location());
  nextToken();  // 'handler'

  consume(FlowToken::Ident);
  std::string name = stringValue();
  if (consumeIf(FlowToken::Semicolon)) {  // forward-declaration
    loc.update(end());
    return std::make_unique<Handler>(name, loc);
  }

  std::unique_ptr<SymbolTable> st = enterScope("handler-" + name);
  std::unique_ptr<Stmt> body = stmt();
  leaveScope();

  if (!body) return nullptr;

  loc.update(body->location().end);

  // forward-declared / previousely -declared?
  if (Handler* handler = scope()->lookup<Handler>(name, Lookup::Self)) {
    if (handler->body() != nullptr) {
      // TODO say where we found the other hand, compared to this one.
      reportError("Redeclaring handler \"%s\"", handler->name().c_str());
      return nullptr;
    }
    handler->implement(std::move(st), std::move(body));
    handler->owner()->removeSymbol(handler);
    return std::unique_ptr<Handler>(handler);
  }

  return std::make_unique<Handler>(name, std::move(st), std::move(body), loc);
}
// }}}
// {{{ expr
std::unique_ptr<Expr> FlowParser::expr() {
  FNTRACE();

  return logicExpr();
}

std::unique_ptr<Expr> FlowParser::logicExpr() {
  FNTRACE();

  std::unique_ptr<Expr> lhs = notExpr();
  if (!lhs) return nullptr;

  for (;;) {
    switch (token()) {
      case FlowToken::And:
      case FlowToken::Xor:
      case FlowToken::Or: {
        FlowToken binop = token();
        nextToken();

        std::unique_ptr<Expr> rhs = notExpr();
        if (!rhs) return nullptr;

        Opcode opc = makeOperator(binop, lhs.get(), rhs.get());
        if (opc == Opcode::EXIT) {
          reportError("Type error in binary expression (%s %s %s).",
                      tos(lhs->getType()).c_str(), binop.c_str(),
                      tos(rhs->getType()).c_str());
          return nullptr;
        }

        lhs = std::make_unique<BinaryExpr>(opc, std::move(lhs), std::move(rhs));
        break;
      }
      default:
        return lhs;
    }
  }
}

std::unique_ptr<Expr> FlowParser::notExpr() {
  FNTRACE();

  size_t nots = 0;

  FlowLocation loc = location();

  while (consumeIf(FlowToken::Not)) nots++;

  std::unique_ptr<Expr> subExpr = relExpr();
  if (!subExpr) return nullptr;

  if ((nots % 2) == 0) return subExpr;

  vm::Opcode op = makeOperator(FlowToken::Not, subExpr.get());
  if (op == Opcode::EXIT) {
    reportError(
        "Type cast error in unary 'not'-operator. Invalid source type <%s>.",
        subExpr->getType());
    return nullptr;
  }

  return std::make_unique<UnaryExpr>(op, std::move(subExpr), loc.update(end()));
}

std::unique_ptr<Expr> FlowParser::relExpr() {
  FNTRACE();

  std::unique_ptr<Expr> lhs = addExpr();
  if (!lhs) return nullptr;

  for (;;) {
    switch (token()) {
      case FlowToken::Equal:
      case FlowToken::UnEqual:
      case FlowToken::Less:
      case FlowToken::Greater:
      case FlowToken::LessOrEqual:
      case FlowToken::GreaterOrEqual:
      case FlowToken::PrefixMatch:
      case FlowToken::SuffixMatch:
      case FlowToken::RegexMatch:
      case FlowToken::In: {
        FlowToken binop = token();
        nextToken();

        std::unique_ptr<Expr> rhs = addExpr();
        if (!rhs) return nullptr;

        Opcode opc = makeOperator(binop, lhs.get(), rhs.get());
        if (opc == Opcode::EXIT) {
          reportError("Type error in binary expression (%s versus %s).",
                      tos(lhs->getType()).c_str(), tos(rhs->getType()).c_str());
          return nullptr;
        }

        lhs = std::make_unique<BinaryExpr>(opc, std::move(lhs), std::move(rhs));
      }
      default:
        return lhs;
    }
  }
}

std::unique_ptr<Expr> FlowParser::addExpr() {
  FNTRACE();

  std::unique_ptr<Expr> lhs = mulExpr();
  if (!lhs) return nullptr;

  for (;;) {
    switch (token()) {
      case FlowToken::Plus:
      case FlowToken::Minus: {
        FlowToken binop = token();
        nextToken();

        std::unique_ptr<Expr> rhs = mulExpr();
        if (!rhs) return nullptr;

        Opcode opc = makeOperator(binop, lhs.get(), rhs.get());
        if (opc == Opcode::EXIT) {
          reportError("Type error in binary expression (%s versus %s).",
                      tos(lhs->getType()).c_str(), tos(rhs->getType()).c_str());
          return nullptr;
        }

        lhs = std::make_unique<BinaryExpr>(opc, std::move(lhs), std::move(rhs));
        break;
      }
      default:
        return lhs;
    }
  }
}

std::unique_ptr<Expr> FlowParser::mulExpr() {
  FNTRACE();

  std::unique_ptr<Expr> lhs = powExpr();
  if (!lhs) return nullptr;

  for (;;) {
    switch (token()) {
      case FlowToken::Mul:
      case FlowToken::Div:
      case FlowToken::Mod:
      case FlowToken::Shl:
      case FlowToken::Shr: {
        FlowToken binop = token();
        nextToken();

        std::unique_ptr<Expr> rhs = powExpr();
        if (!rhs) return nullptr;

        Opcode opc = makeOperator(binop, lhs.get(), rhs.get());
        if (opc == Opcode::EXIT) {
          reportError("Type error in binary expression (%s versus %s).",
                      tos(lhs->getType()).c_str(), tos(rhs->getType()).c_str());
          return nullptr;
        }

        lhs = std::make_unique<BinaryExpr>(opc, std::move(lhs), std::move(rhs));
        break;
      }
      default:
        return lhs;
    }
  }
}

std::unique_ptr<Expr> FlowParser::powExpr() {
  // powExpr ::= negExpr ('**' powExpr)*
  FNTRACE();

  FlowLocation sloc(location());
  std::unique_ptr<Expr> left = negExpr();
  if (!left) return nullptr;

  while (token() == FlowToken::Pow) {
    nextToken();

    std::unique_ptr<Expr> right = powExpr();
    if (!right) return nullptr;

    auto opc = makeOperator(FlowToken::Pow, left.get(), right.get());
    if (opc == Opcode::EXIT) {
      reportError("Type error in binary expression (%s versus %s).",
                  tos(left->getType()).c_str(), tos(right->getType()).c_str());
      return nullptr;
    }

    left = std::make_unique<BinaryExpr>(opc, std::move(left), std::move(right));
  }

  return left;
}

std::unique_ptr<Expr> FlowParser::negExpr() {
  // negExpr ::= ['-'] primaryExpr
  FNTRACE();

  FlowLocation loc = location();

  if (consumeIf(FlowToken::Minus)) {
    std::unique_ptr<Expr> e = negExpr();
    if (!e) return nullptr;

    vm::Opcode op = makeOperator(FlowToken::Minus, e.get());
    if (op == Opcode::EXIT) {
      reportError(
          "Type cast error in unary 'neg'-operator. Invalid source type <%s>.",
          e->getType());
      return nullptr;
    }

    e = std::make_unique<UnaryExpr>(op, std::move(e), loc.update(end()));
    return e;
  } else {
    return bitNotExpr();
  }
}

std::unique_ptr<Expr> FlowParser::bitNotExpr() {
  // negExpr ::= ['~'] primaryExpr
  FNTRACE();

  FlowLocation loc = location();

  if (consumeIf(FlowToken::BitNot)) {
    std::unique_ptr<Expr> e = bitNotExpr();
    if (!e) return nullptr;

    vm::Opcode op = makeOperator(FlowToken::BitNot, e.get());
    if (op == Opcode::EXIT) {
      reportError(
          "Type cast error in unary 'not'-operator. Invalid source type <%s>.",
          e->getType());
      return nullptr;
    }

    e = std::make_unique<UnaryExpr>(op, std::move(e), loc.update(end()));
    return e;
  } else {
    return primaryExpr();
  }
}

// primaryExpr ::= literalExpr
//               | variable
//               | function '(' paramList ')'
//               | '(' expr ')'
//               | '[' exprList ']'
std::unique_ptr<Expr> FlowParser::primaryExpr() {
  FNTRACE();

  switch (token()) {
    case FlowToken::String:
    case FlowToken::RawString:
    case FlowToken::Number:
    case FlowToken::Boolean:
    case FlowToken::IP:
    case FlowToken::Cidr:
    case FlowToken::RegExp:
      return literalExpr();
    case FlowToken::StringType:
    case FlowToken::NumberType:
    case FlowToken::BoolType:
      return castExpr();
    case FlowToken::InterpolatedStringFragment:
      return interpolatedStr();
    case FlowToken::Ident: {
      FlowLocation loc = location();
      std::string name = stringValue();
      nextToken();

      std::list<Symbol*> symbols;
      Symbol* symbol = scope()->lookup(name, Lookup::All, &symbols);
      if (!symbol) {
        // XXX assume that given symbol is a auto forward-declared handler.
        Handler* href = (Handler*)globalScope()->appendSymbol(
            std::make_unique<Handler>(name, loc));
        return std::make_unique<HandlerRefExpr>(href, loc);
      }

      if (auto variable = dynamic_cast<Variable*>(symbol))
        return std::make_unique<VariableExpr>(variable, loc);

      if (auto handler = dynamic_cast<Handler*>(symbol))
        return std::make_unique<HandlerRefExpr>(handler, loc);

      if (symbol->type() == Symbol::BuiltinFunction) {
        std::list<Callable*> callables;
        for (Symbol* s : symbols) {
          if (auto c = dynamic_cast<BuiltinFunction*>(s)) {
            callables.push_back(c);
          }
        }

        ParamList params;
        // {{{ parse call params
        if (token() == FlowToken::RndOpen) {
          nextToken();
          if (token() != FlowToken::RndClose) {
            auto ra = paramList();
            if (!ra) {
              return nullptr;
            }
            params = std::move(*ra);
          }
          loc.end = lastLocation().end;
          if (!consume(FlowToken::RndClose)) {
            return nullptr;
          }
        } else if (FlowTokenTraits::isUnaryOp(token()) ||
                   FlowTokenTraits::isLiteral(token()) ||
                   token() == FlowToken::Ident ||
                   token() == FlowToken::BrOpen ||
                   token() == FlowToken::RndOpen) {
          auto ra = paramList();
          if (!ra) {
            return nullptr;
          }
          params = std::move(*ra);
          loc.end = params.location().end;
        }
        // }}}

        return resolve(callables, std::move(params));
      }

      reportError("Unsupported symbol type of \"%s\" in expression.",
                  name.c_str());
      return nullptr;
    }
    case FlowToken::Begin: {  // lambda-like inline function ref
      char name[64];
      static unsigned long i = 0;
      ++i;
      snprintf(name, sizeof(name), "__lambda_%lu", i);

      FlowLocation loc = location();
      auto st = std::make_unique<SymbolTable>(scope(), name);
      enter(st.get());
      std::unique_ptr<Stmt> body = compoundStmt();
      leave();

      if (!body) return nullptr;

      loc.update(body->location().end);

      Handler* handler = new Handler(name, std::move(st), std::move(body), loc);
      // TODO (memory leak): add handler to unit's global scope, i.e. via:
      //       - scope()->rootScope()->insert(handler);
      //       - unit_->scope()->insert(handler);
      //       to get free'd
      return std::make_unique<HandlerRefExpr>(handler, loc);
    }
    case FlowToken::RndOpen: {
      FlowLocation loc = location();
      nextToken();
      std::unique_ptr<Expr> e = expr();
      consume(FlowToken::RndClose);
      if (e) {
        e->setLocation(loc.update(end()));
      }
      return e;
    }
    case FlowToken::BrOpen:
      return arrayExpr();
    default:
      TRACE(1, "Expected primary expression. Got something... else.");
      reportUnexpectedToken();
      return nullptr;
  }
}

std::unique_ptr<Expr> FlowParser::arrayExpr() {
  FlowLocation loc = location();
  nextToken();  // '['
  std::vector<std::unique_ptr<Expr>> fields;

  if (token() != FlowToken::BrClose) {
    auto e = expr();
    if (!e) return nullptr;

    fields.push_back(std::move(e));

    while (consumeIf(FlowToken::Comma)) {
      e = expr();
      if (!e) return nullptr;

      fields.push_back(std::move(e));
    }
  }

  consume(FlowToken::BrClose);

  if (!fields.empty()) {
    FlowType baseType = fields.front()->getType();
    for (const auto& e : fields) {
      if (e->getType() != baseType) {
        reportError("Mixed element types in array not allowed.");
        return nullptr;
      }
    }

    switch (baseType) {
      case FlowType::Number:
      case FlowType::String:
      case FlowType::IPAddress:
      case FlowType::Cidr:
        break;
      default:
        reportError(
            "Invalid array expression. Element type <%s> is not allowed.",
            tos(baseType).c_str());
        return nullptr;
    }
  } else {
    reportError("Empty arrays are not allowed.");
    return nullptr;
  }

  return std::make_unique<ArrayExpr>(loc.update(end()), std::move(fields));
}

std::unique_ptr<Expr> FlowParser::literalExpr() {
  FNTRACE();

  // literalExpr  ::= NUMBER [UNIT]
  //                | BOOL
  //                | STRING
  //                | IP_ADDR
  //                | IP_CIDR
  //                | REGEXP

  static struct {
    const char* ident;
    long long nominator;
    long long denominator;
  } units[] = {{"byte", 1, 1},
               {"kbyte", 1024llu, 1},
               {"mbyte", 1024llu * 1024, 1},
               {"gbyte", 1024llu * 1024 * 1024, 1},
               {"tbyte", 1024llu * 1024 * 1024 * 1024, 1},
               {"bit", 1, 8},
               {"kbit", 1024llu, 8},
               {"mbit", 1024llu * 1024, 8},
               {"gbit", 1024llu * 1024 * 1024, 8},
               {"tbit", 1024llu * 1024 * 1024 * 1024, 8},
               {"sec", 1, 1},
               {"min", 60llu, 1},
               {"hour", 60llu * 60, 1},
               {"day", 60llu * 60 * 24, 1},
               {"week", 60llu * 60 * 24 * 7, 1},
               {"month", 60llu * 60 * 24 * 30, 1},
               {"year", 60llu * 60 * 24 * 365, 1},
               {nullptr, 1, 1}};

  FlowLocation loc(location());

  switch (token()) {
    case FlowToken::Div: {  // /REGEX/
      if (lexer_->continueParseRegEx('/')) {
        auto e = std::make_unique<RegExpExpr>(RegExp(stringValue()),
                                              loc.update(end()));
        nextToken();
        return std::move(e);
      } else {
        reportError("Error parsing regular expression.");
        return nullptr;
      }
    }
    case FlowToken::Number: {  // NUMBER [UNIT]
      auto number = numberValue();
      nextToken();

      if (token() == FlowToken::Ident) {
        std::string sv(stringValue());
        for (size_t i = 0; units[i].ident; ++i) {
          if (sv == units[i].ident ||
              (sv[sv.size() - 1] == 's' &&
               sv.substr(0, sv.size() - 1) == units[i].ident)) {
            nextToken();  // UNIT
            number = number * units[i].nominator / units[i].denominator;
            loc.update(end());
            break;
          }
        }
      }
      return std::make_unique<NumberExpr>(number, loc);
    }
    case FlowToken::Boolean: {
      std::unique_ptr<BoolExpr> e =
          std::make_unique<BoolExpr>(booleanValue(), loc);
      nextToken();
      return std::move(e);
    }
    case FlowToken::String:
    case FlowToken::RawString: {
      std::unique_ptr<StringExpr> e =
          std::make_unique<StringExpr>(stringValue(), loc);
      nextToken();
      return std::move(e);
    }
    case FlowToken::IP: {
      std::unique_ptr<IPAddressExpr> e =
          std::make_unique<IPAddressExpr>(lexer_->ipValue(), loc);
      nextToken();
      return std::move(e);
    }
    case FlowToken::Cidr: {
      std::unique_ptr<CidrExpr> e =
          std::make_unique<CidrExpr>(lexer_->cidr(), loc);
      nextToken();
      return std::move(e);
    }
    case FlowToken::RegExp: {
      std::unique_ptr<RegExpExpr> e =
          std::make_unique<RegExpExpr>(RegExp(stringValue()), loc);
      nextToken();
      return std::move(e);
    }
    default:
      reportError("Expected literal expression, but got %s.", token().c_str());
      return nullptr;
  }
}

std::unique_ptr<ParamList> FlowParser::paramList() {
  // paramList       ::= namedExpr *(',' namedExpr)
  //                   | expr *(',' expr)

  FNTRACE();

  if (token() == FlowToken::NamedParam) {
    std::unique_ptr<ParamList> args(new ParamList(true));
    std::string name;
    std::unique_ptr<Expr> e = namedExpr(&name);
    if (!e) return nullptr;

    args->push_back(name, std::move(e));

    while (token() == FlowToken::Comma) {
      nextToken();

      if (token() == FlowToken::RndClose) break;

      e = namedExpr(&name);
      if (!e) return nullptr;

      args->push_back(name, std::move(e));
    }
    return args;
  } else {
    // unnamed param list
    std::unique_ptr<ParamList> args(new ParamList(false));

    std::unique_ptr<Expr> e = expr();
    if (!e) return nullptr;

    args->push_back(std::move(e));

    while (token() == FlowToken::Comma) {
      nextToken();

      if (token() == FlowToken::RndClose) break;

      e = expr();
      if (!e) return nullptr;

      args->push_back(std::move(e));
    }
    return args;
  }
}

std::unique_ptr<Expr> FlowParser::namedExpr(std::string* name) {
  // namedExpr       ::= NAMED_PARAM expr

  // FIXME: wtf? what way around?

  *name = stringValue();
  if (!consume(FlowToken::NamedParam)) return nullptr;

  return expr();
}

std::unique_ptr<Expr> asString(std::unique_ptr<Expr>&& expr) {
  FlowType baseType = expr->getType();
  if (baseType == FlowType::String) return std::move(expr);

  Opcode opc = makeOperator(FlowToken::StringType, expr.get());
  if (opc == Opcode::EXIT) return nullptr;  // cast error

  return std::make_unique<UnaryExpr>(opc, std::move(expr), expr->location());
}

std::unique_ptr<Expr> FlowParser::interpolatedStr() {
  FNTRACE();

  FlowLocation sloc(location());
  std::unique_ptr<Expr> result =
      std::make_unique<StringExpr>(stringValue(), sloc.update(end()));
  nextToken();  // interpolation start

  std::unique_ptr<Expr> e(expr());
  if (!e) return nullptr;

  e = asString(std::move(e));
  if (!e) {
    reportError("Cast error in string interpolation.");
    return nullptr;
  }

  result = std::make_unique<BinaryExpr>(Opcode::SADD, std::move(result),
                                        std::move(e));

  while (token() == FlowToken::InterpolatedStringFragment) {
    FlowLocation tloc = sloc.update(end());
    result = std::make_unique<BinaryExpr>(
        Opcode::SADD, std::move(result),
        std::make_unique<StringExpr>(stringValue(), tloc));
    nextToken();

    e = expr();
    if (!e) return nullptr;

    e = asString(std::move(e));
    if (!e) {
      reportError("Cast error in string interpolation.");
      return nullptr;
    }

    result = std::make_unique<BinaryExpr>(Opcode::SADD, std::move(result),
                                          std::move(e));
  }

  if (!expect(FlowToken::InterpolatedStringEnd)) {
    return nullptr;
  }

  if (!stringValue().empty()) {
    result = std::make_unique<BinaryExpr>(
        Opcode::SADD, std::move(result),
        std::make_unique<StringExpr>(stringValue(), sloc.update(end())));
  }

  nextToken();  // skip InterpolatedStringEnd

  return result;
}

// castExpr ::= 'int' '(' expr ')'
//            | 'string' '(' expr ')'
//            | 'bool' '(' expr ')'
std::unique_ptr<Expr> FlowParser::castExpr() {
  FNTRACE();

  FlowLocation sloc(location());

  FlowToken targetTypeToken = token();
  nextToken();

  if (!consume(FlowToken::RndOpen)) return nullptr;

  std::unique_ptr<Expr> e(expr());

  if (!consume(FlowToken::RndClose)) return nullptr;

  if (!e) return nullptr;

  Opcode targetType = makeOperator(targetTypeToken, e.get());
  if (targetType == Opcode::EXIT) {
    reportError(
        "Type cast error. No cast implementation found for requested cast from "
        "%s to %s.",
        tos(e->getType()).c_str(), targetTypeToken.c_str());
    return nullptr;
  }

  if (targetType == Opcode::NOP) {
    return e;
  }

  // printf("Type cast from %s to %s: %s\n", tos(e->getType()).c_str(),
  // targetTypeToken.c_str(), mnemonic(targetType));
  return std::make_unique<UnaryExpr>(targetType, std::move(e),
                                     sloc.update(end()));
}
// }}}
// {{{ stmt
std::unique_ptr<Stmt> FlowParser::stmt() {
  FNTRACE();

  switch (token()) {
    case FlowToken::If:
      return ifStmt();
    case FlowToken::Match:
      return matchStmt();
    case FlowToken::Begin:
      return compoundStmt();
    case FlowToken::Ident:
      return identStmt();
    case FlowToken::Semicolon: {
      FlowLocation sloc(location());
      nextToken();
      return std::make_unique<CompoundStmt>(sloc.update(end()));
    }
    default:
      reportError("Unexpected token \"%s\". Expected a statement instead.",
                  token().c_str());
      return nullptr;
  }
}

std::unique_ptr<Stmt> FlowParser::ifStmt() {
  // ifStmt ::= 'if' expr ['then'] stmt ['else' stmt]
  FNTRACE();
  FlowLocation sloc(location());

  consume(FlowToken::If);
  std::unique_ptr<Expr> cond(expr());
  consumeIf(FlowToken::Then);

  std::unique_ptr<Stmt> thenStmt(stmt());
  if (!thenStmt) return nullptr;

  std::unique_ptr<Stmt> elseStmt;

  if (consumeIf(FlowToken::Else)) {
    elseStmt = std::move(stmt());
    if (!elseStmt) {
      return nullptr;
    }
  }

  return std::make_unique<CondStmt>(std::move(cond), std::move(thenStmt),
                                    std::move(elseStmt), sloc.update(end()));
}

std::unique_ptr<Stmt> FlowParser::matchStmt() {
  FNTRACE();

  // matchStmt       ::= 'match' expr [MATCH_OP] '{' *matchCase ['else' stmt]
  // '}'
  // matchCase       ::= 'on' literalExpr *(',' 'on' literalExpr) stmt
  // MATCH_OP        ::= '==' | '=^' | '=$' | '=~'

  FlowLocation sloc(location());

  if (!consume(FlowToken::Match)) return nullptr;

  auto cond = addExpr();
  if (!cond) return nullptr;

  FlowType matchType = cond->getType();

  if (matchType != FlowType::String) {
    reportError("Expected match condition type <%s>, found \"%s\" instead.",
                tos(FlowType::String).c_str(), tos(matchType).c_str());
    return nullptr;
  }

  // [MATCH_OP]
  vm::MatchClass op;
  if (FlowTokenTraits::isOperator(token())) {
    switch (token()) {
      case FlowToken::Equal:
        op = vm::MatchClass::Same;
        break;
      case FlowToken::PrefixMatch:
        op = vm::MatchClass::Head;
        break;
      case FlowToken::SuffixMatch:
        op = vm::MatchClass::Tail;
        break;
      case FlowToken::RegexMatch:
        op = vm::MatchClass::RegExp;
        break;
      default:
        reportError("Expected match oeprator, found \"%s\" instead.",
                    token().c_str());
        return nullptr;
    }
    nextToken();
  } else {
    op = vm::MatchClass::Same;
  }

  if (op == vm::MatchClass::RegExp) matchType = FlowType::RegExp;

  // '{'
  if (!consume(FlowToken::Begin)) return nullptr;

  // *('on' literalExpr *(',' 'on' literalExpr) stmt)
  MatchStmt::CaseList cases;
  do {
    if (!consume(FlowToken::On)) {
      return nullptr;
    }

    MatchCase one;

    // first label
    auto label = literalExpr();
    if (!label) return nullptr;

    one.first.push_back(std::move(label));

    // any more labels
    while (consumeIf(FlowToken::Comma)) {
      if (!consume(FlowToken::On)) return nullptr;

      label = literalExpr();
      if (!label) return nullptr;

      one.first.push_back(std::move(label));
    }

    for (auto& label : one.first) {
      FlowType caseType = label->getType();
      if (matchType != caseType) {
        reportError(
            "Type mismatch in match-on statement. Expected <%s> but got <%s>.",
            tos(matchType).c_str(), tos(caseType).c_str());
        return nullptr;
      }
    }

    one.second = stmt();
    if (!one.second) return nullptr;

    cases.push_back(std::move(one));
  } while (token() == FlowToken::On);

  // ['else' stmt]
  std::unique_ptr<Stmt> elseStmt;
  if (consumeIf(FlowToken::Else)) {
    elseStmt = stmt();
    if (!elseStmt) {
      return nullptr;
    }
  }

  // '}'
  if (!consume(FlowToken::End)) return nullptr;

  return std::make_unique<MatchStmt>(sloc.update(end()), std::move(cond), op,
                                     std::move(cases), std::move(elseStmt));
}

// compoundStmt ::= '{' varDecl* stmt* '}'
std::unique_ptr<Stmt> FlowParser::compoundStmt() {
  FNTRACE();
  FlowLocation sloc(location());
  nextToken();  // '{'

  std::unique_ptr<CompoundStmt> cs = std::make_unique<CompoundStmt>(sloc);

  while (token() == FlowToken::Var) {
    if (std::unique_ptr<Variable> var = varDecl())
      scope()->appendSymbol(std::move(var));
    else
      return nullptr;
  }

  for (;;) {
    if (consumeIf(FlowToken::End)) {
      cs->location().update(end());
      return std::unique_ptr<Stmt>(cs.release());
    }

    if (std::unique_ptr<Stmt> s = stmt())
      cs->push_back(std::move(s));
    else
      return nullptr;
  }
}

std::unique_ptr<Stmt> FlowParser::identStmt() {
  FNTRACE();

  // identStmt  ::= callStmt | assignStmt
  // callStmt   ::= NAME ['(' paramList ')' | paramList] (';' | LF)
  // assignStmt ::= NAME '=' expr [';' | LF]
  //
  // NAME may be a builtin-function, builtin-handler, handler-name, or variable.

  FlowLocation loc(location());
  std::string name = stringValue();
  nextToken();  // IDENT

  std::unique_ptr<Stmt> stmt;
  std::list<Symbol*> symbols;
  Symbol* callee = scope()->lookup(name, Lookup::All, &symbols);
  if (!callee) {
    // XXX assume that given symbol is a auto forward-declared handler that's
    // being defined later in the source.
    if (token() != FlowToken::Semicolon) {
      reportError("Unknown symbol '%s'.", name.c_str());
      return nullptr;
    }

    callee = (Handler*)globalScope()->appendSymbol(
        std::make_unique<Handler>(name, loc));
    symbols.push_back(callee);
  }

  switch (callee->type()) {
    case Symbol::Variable: {  // var '=' expr (';' | LF)
      if (!consume(FlowToken::Assign)) return nullptr;

      std::unique_ptr<Expr> value = expr();
      if (!value) return nullptr;

      Variable* var = static_cast<Variable*>(callee);
      FlowType leftType = var->initializer()->getType();
      FlowType rightType = value->getType();
      if (leftType != rightType) {
        reportError("Type mismatch in assignment. Expected <%s> but got <%s>.",
                    tos(leftType).c_str(), tos(rightType).c_str());
        return nullptr;
      }

      stmt = std::make_unique<AssignStmt>(var, std::move(value),
                                          loc.update(end()));
      break;
    }
    case Symbol::BuiltinFunction:
    case Symbol::BuiltinHandler: {
      auto call = callStmt(symbols);
      if (!call) {
        return nullptr;
      }
      stmt = std::make_unique<ExprStmt>(std::move(call));
      break;
    }
    case Symbol::Handler:
      stmt = std::make_unique<ExprStmt>(
          std::make_unique<CallExpr>(loc, (Callable*)callee, ParamList()));
      break;
    default:
      break;
  }

  // postscript statement handling

  switch (token()) {
    case FlowToken::If:
    case FlowToken::Unless:
      return postscriptStmt(std::move(stmt));
  }

  if (!consume(FlowToken::Semicolon)) return nullptr;

  return stmt;
}

std::unique_ptr<CallExpr> FlowParser::callStmt(
    const std::list<Symbol*>& symbols) {
  FNTRACE();

  // callStmt ::= NAME ['(' paramList ')' | paramList] (';' | LF)
  // namedArg ::= NAME ':' expr

  std::list<Callable*> callables;
  for (Symbol* s : symbols) {
    if (auto c = dynamic_cast<Callable*>(s)) {
      callables.push_back(c);
    }
  }

  if (callables.empty()) {
    reportError("Symbol is not callable.");  // XXX should never reach here
    return nullptr;
  }

  ParamList params;
  FlowLocation loc = location();

  // {{{ parse call params
  if (token() == FlowToken::RndOpen) {
    nextToken();
    if (token() != FlowToken::RndClose) {
      auto ra = paramList();
      if (!ra) {
        return nullptr;
      }
      params = std::move(*ra);
    }
    loc.end = lastLocation().end;
    if (!consume(FlowToken::RndClose)) {
      return nullptr;
    }
  } else if (token() != FlowToken::Semicolon && token() != FlowToken::If &&
             token() != FlowToken::Unless) {
    auto ra = paramList();
    if (!ra) {
      return nullptr;
    }
    params = std::move(*ra);
    loc.end = params.location().end;
  }
  // }}}

  return resolve(callables, std::move(params));
}

vm::Signature makeSignature(const Callable* callee,
                                const ParamList& params) {
  vm::Signature sig;

  sig.setName(callee->name());

  std::vector<FlowType> argTypes;
  for (const Expr* arg : params.values()) {
    argTypes.push_back(arg->getType());
  }

  sig.setArgs(argTypes);

  return sig;
};

std::unique_ptr<CallExpr> FlowParser::resolve(
    const std::list<Callable*>& callables, ParamList&& params) {
  FNTRACE();

  auto inputSignature = makeSignature(callables.front(), params);

  // attempt to find a full match first
  for (Callable* callee : callables) {
    if (callee->isDirectMatch(params)) {
      return std::make_unique<CallExpr>(callee->location(), callee,
                                        std::move(params));
    }
  }

  // attempt to find something with default values or parameter-reordering (if
  // named args)
  std::list<Callable*> result;
  std::list<std::pair<Callable*, Buffer>> matchErrors;

  for (Callable* callee : callables) {
    Buffer msg;
    if (callee->tryMatch(params, &msg)) {
      result.push_back(callee);
    } else {
      matchErrors.push_back(std::make_pair(callee, msg));
    }
  }

  if (result.empty()) {
    reportError("No matching signature for %s.", inputSignature.to_s().c_str());
    for (const auto& me : matchErrors) {
      reportError("Possible candidate %s failed. %s",
                  me.first->signature().to_s().c_str(), me.second.c_str());
    }
    return nullptr;
  }

  if (result.size() > 1) {
    reportError("Call to builtin is ambiguous.");
    return nullptr;
  }

  return std::make_unique<CallExpr>(result.front()->location(), result.front(),
                                    std::move(params));
}

std::unique_ptr<Stmt> FlowParser::postscriptStmt(
    std::unique_ptr<Stmt> baseStmt) {
  FNTRACE();

  FlowToken op = token();
  switch (op) {
    case FlowToken::If:
    case FlowToken::Unless:
      break;
    default:
      return baseStmt;
  }

  // STMT ['if' EXPR] ';'
  // STMT ['unless' EXPR] ';'

  FlowLocation sloc = location();

  nextToken();  // 'if' | 'unless'

  std::unique_ptr<Expr> condExpr = expr();
  if (!condExpr) return nullptr;

  if (op == FlowToken::Unless) {
    auto opc = makeOperator(FlowToken::Not, condExpr.get());
    if (opc == Opcode::EXIT) {
      reportError(
          "Type cast error. No cast implementation found for requested cast "
          "from %s to %s.",
          tos(condExpr->getType()).c_str(), "bool");
      return nullptr;
    }

    condExpr = std::make_unique<UnaryExpr>(opc, std::move(condExpr), sloc);
  }

  if (!consume(FlowToken::Semicolon)) return nullptr;

  return std::make_unique<CondStmt>(std::move(condExpr), std::move(baseStmt),
                                    nullptr, sloc.update(end()));
}
// }}}

}  // namespace flow
}  // namespace xzero
