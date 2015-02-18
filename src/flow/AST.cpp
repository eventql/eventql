// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/AST.h>
#include <xzero/flow/ASTPrinter.h>
#include <xzero/flow/vm/Signature.h>
#include <xzero/flow/vm/NativeCallback.h>
#include <xzero/Buffer.h>
#include <xzero/Utility.h> // make_unique
#include <algorithm>
#include <cstdlib>
#include <cstdio>

namespace xzero {
namespace flow {

// {{{ SymbolTable
SymbolTable::SymbolTable(SymbolTable* outer, const std::string& name)
    : symbols_(), outerTable_(outer), name_(name) {}

SymbolTable::~SymbolTable() {
  for (auto symbol : symbols_) delete symbol;
}

Symbol* SymbolTable::appendSymbol(std::unique_ptr<Symbol> symbol) {
  assert(symbol->owner_ == nullptr && "Cannot re-own symbol.");
  symbol->owner_ = this;
  symbols_.push_back(symbol.get());
  return symbol.release();
}

void SymbolTable::removeSymbol(Symbol* symbol) {
  auto i = std::find(symbols_.begin(), symbols_.end(), symbol);

  assert(i != symbols_.end() && "Failed removing symbol from symbol table.");

  symbols_.erase(i);
  symbol->owner_ = nullptr;
}

Symbol* SymbolTable::symbolAt(size_t i) const { return symbols_[i]; }

size_t SymbolTable::symbolCount() const { return symbols_.size(); }

Symbol* SymbolTable::lookup(const std::string& name, Lookup method) const {
  // search local
  if (method & Lookup::Self)
    for (auto symbol : symbols_)
      if (symbol->name() == name) {
        // printf("SymbolTable(%s).lookup: \"%s\" (found)\n", name_.c_str(),
        // name.c_str());
        return symbol;
      }

  // search outer
  if (method & Lookup::Outer)
    if (outerTable_) {
      // printf("SymbolTable(%s).lookup: \"%s\" (not found, recurse to
      // parent)\n", name_.c_str(), name.c_str());
      return outerTable_->lookup(name, method);
    }

  // printf("SymbolTable(%s).lookup: \"%s\" -> not found\n", name_.c_str(),
  // name.c_str());
  return nullptr;
}

Symbol* SymbolTable::lookup(const std::string& name, Lookup method,
                            std::list<Symbol*>* result) const {
  assert(result != nullptr);

  // search local
  if (method & Lookup::Self)
    for (auto symbol : symbols_)
      if (symbol->name() == name) result->push_back(symbol);

  // search outer
  if (method & Lookup::Outer)
    if (outerTable_) outerTable_->lookup(name, method, result);

  return !result->empty() ? result->front() : nullptr;
}
// }}}
// {{{ Callable
Callable::Callable(Type t, const vm::NativeCallback* cb,
                   const FlowLocation& loc)
    : Symbol(t, cb->signature().name(), loc),
      nativeCallback_(cb),
      sig_(nativeCallback_->signature()) {}

Callable::Callable(const std::string& name, const FlowLocation& loc)
    : Symbol(Type::Handler, name, loc), nativeCallback_(nullptr), sig_() {
  sig_.setName(name);
  sig_.setReturnType(FlowType::Boolean);
}

const vm::Signature& Callable::signature() const {
  if (nativeCallback_) return nativeCallback_->signature();

  return sig_;
}

static inline void completeDefaultValue(ParamList& args, FlowType type,
                                        const void* defaultValue,
                                        const std::string& name)  // {{{
{
  // printf("completeDefaultValue(type:%s, name:%s)\n", tos(type).c_str(),
  // name.c_str());

  static const FlowLocation loc;

  switch (type) {
    case FlowType::Boolean:
      if (args.isNamed())
        args.push_back(name,
                       std::make_unique<BoolExpr>(*(bool*)defaultValue, loc));
      else
        args.push_back(std::make_unique<BoolExpr>(*(bool*)defaultValue, loc));
      break;
    case FlowType::Number:
      if (args.isNamed())
        args.push_back(name, std::make_unique<NumberExpr>(
                                 *(FlowNumber*)defaultValue, loc));
      else
        args.push_back(
            std::make_unique<NumberExpr>(*(FlowNumber*)defaultValue, loc));
      break;
    case FlowType::String: {
      const FlowString* s = (FlowString*)defaultValue;
      // printf("auto-complete parameter \"%s\" <%s> = \"%s\"\n", name.c_str(),
      // tos(type).c_str(), s->str().c_str());
      if (args.isNamed())
        args.push_back(name, std::make_unique<StringExpr>(s->str(), loc));
      else
        args.push_back(std::make_unique<StringExpr>(s->str(), loc));
      break;
    }
    case FlowType::IPAddress:
      if (args.isNamed())
        args.push_back(name, std::make_unique<IPAddressExpr>(
                                 *(IPAddress*)defaultValue, loc));
      else
        args.push_back(
            std::make_unique<IPAddressExpr>(*(IPAddress*)defaultValue, loc));
      break;
    case FlowType::Cidr:
      if (args.isNamed())
        args.push_back(name,
                       std::make_unique<CidrExpr>(*(Cidr*)defaultValue, loc));
      else
        args.push_back(std::make_unique<CidrExpr>(*(Cidr*)defaultValue, loc));
      break;
    default:
      fprintf(stderr,
              "Unsupported type in default completion. Please report me. I am "
              "a bug.\n");
      abort();
      // reportError("Cannot complete named paramter \"%s\" in callee \"%s\".
      // Unsupported type <%s>.",
      //        name.c_str(), this->name().c_str(), tos(type).c_str());
      break;
  }
}  // }}}

bool Callable::isDirectMatch(const ParamList& params) const {
  if (params.size() != nativeCallback_->signature().args().size()) return false;

  for (size_t i = 0, e = params.size(); i != e; ++i) {
    if (params.isNamed() && nativeCallback_->getNameAt(i) != params[i].first)
      return false;

    FlowType expectedType = signature().args()[i];
    FlowType givenType = params.values()[i]->getType();

    if (givenType != expectedType) {
      return false;
    }
  }
  return true;
}

bool Callable::tryMatch(ParamList& params, Buffer* errorMessage) const {
  // printf("Callable(%s).tryMatch()\n", name().c_str());

  const vm::NativeCallback* native = nativeCallback();

  if (params.empty() && (!native || native->signature().args().empty()))
    return true;

  if (params.isNamed()) {
    if (!native->isNamed()) {
      errorMessage->printf(
          "Callee \"%s\" invoked with named parameters, but no names provided "
          "by runtime.",
          name().c_str());
      return false;
    }

    int argc = signature().args().size();
    for (int i = 0; i != argc; ++i) {
      const auto& name = native->getNameAt(i);
      if (!params.contains(name)) {
        const void* defaultValue = native->getDefaultAt(i);
        if (!defaultValue) {
          errorMessage->printf(
              "Callee \"%s\" invoked without required named parameter \"%s\".",
              this->name().c_str(), name.c_str());
          return false;
        }
        FlowType type = signature().args()[i];
        completeDefaultValue(params, type, defaultValue, name);
      }
    }

    // reorder params (and detect superfluous params)
    std::vector<std::string> superfluous;
    params.reorder(native, &superfluous);

    if (!superfluous.empty()) {
      std::string t;
      for (const auto& s : superfluous) {
        if (!t.empty()) t += ", ";
        t += "\"";
        t += s;
        t += "\"";
      }
      errorMessage->printf("Superfluous arguments passed to callee \"%s\": %s.",
                           this->name().c_str(), t.c_str());
      return false;
    }

    return true;
  } else  // verify params positional
  {
    if (params.size() > signature().args().size()) {
      errorMessage->printf("Superfluous parameters to callee %s.",
                           signature().to_s().c_str());
      return false;
    }

    for (size_t i = 0, e = params.size(); i != e; ++i) {
      FlowType expectedType = signature().args()[i];
      FlowType givenType = params.values()[i]->getType();
      if (givenType != expectedType) {
        errorMessage->printf(
            "Type mismatch in positional parameter %d, callee %s.", i + 1,
            signature().to_s().c_str());
        return false;
      }
    }

    for (size_t i = params.size(), e = signature().args().size(); i != e; ++i) {
      const void* defaultValue = native->getDefaultAt(i);
      if (!defaultValue) {
        errorMessage->printf(
            "No default value provided for positional parameter %d, callee %s.",
            i + 1, signature().to_s().c_str());
        return false;
      }

      const std::string& name = native->getNameAt(i);
      FlowType type = native->signature().args()[i];
      completeDefaultValue(params, type, defaultValue, name);
    }

    vm::Signature sig;
    sig.setName(this->name());
    sig.setReturnType(signature().returnType());  // XXX cheetah
    std::vector<FlowType> argTypes;
    for (const auto& arg : params.values()) {
      argTypes.push_back(arg->getType());
    }
    sig.setArgs(argTypes);

    if (sig != signature()) {
      errorMessage->printf(
          "Callee parameter type signature mismatch: %s passed, but %s "
          "expected.",
          sig.to_s().c_str(), signature().to_s().c_str());
      return false;
    }

    return true;
  }
}
// }}}
// {{{ ParamList
ParamList& ParamList::operator=(ParamList&& v) {
  isNamed_ = v.isNamed_;
  names_ = std::move(v.names_);
  values_ = std::move(v.values_);

  return *this;
}

ParamList::~ParamList() {
  for (Expr* arg : values_) {
    delete arg;
  }
}

void ParamList::push_back(const std::string& name,
                          std::unique_ptr<Expr>&& arg) {
  assert(names_.size() == values_.size() &&
         "Cannot mix named with unnamed parameters.");
  assert(names_.size() == values_.size());

  names_.push_back(name);
  values_.push_back(arg.release());
}

void ParamList::push_back(std::unique_ptr<Expr>&& arg) {
  assert(names_.empty() && "Cannot mix unnamed with named parameters.");

  values_.push_back(arg.release());
}

void ParamList::replace(size_t index, std::unique_ptr<Expr>&& value) {
  assert(index < values_.size() && "Index out of bounds.");

  delete values_[index];
  values_[index] = value.release();
}

bool ParamList::replace(const std::string& name,
                        std::unique_ptr<Expr>&& value) {
  assert(!names_.empty() && "Cannot mix unnamed with named parameters.");
  assert(names_.size() == values_.size());

  for (size_t i = 0, e = names_.size(); i != e; ++i) {
    if (names_[i] == name) {
      delete values_[i];
      values_[i] = value.release();
      return true;
    }
  }

  names_.push_back(name);
  values_.push_back(value.release());
  return false;
}

bool ParamList::contains(const std::string& name) const {
  for (const auto& arg : names_)
    if (name == arg) return true;

  return false;
}

void ParamList::swap(size_t source, size_t dest) {
  assert(source <= size());
  assert(dest <= size());

  // printf("Swapping parameter #%zu (%s) with #%zu (%s).\n", source,
  // names_[source].c_str(), dest, names_[dest].c_str());

  std::swap(names_[source], names_[dest]);
  std::swap(values_[source], values_[dest]);
}

size_t ParamList::size() const { return values_.size(); }

bool ParamList::empty() const { return values_.empty(); }

std::pair<std::string, Expr*> ParamList::at(size_t offset) const {
  return std::make_pair(isNamed() ? names_[offset] : "", values_[offset]);
}

void ParamList::reorder(const vm::NativeCallback* native,
                        std::vector<std::string>* superfluous) {
  // dump("ParamList::reorder (before)");

  size_t argc = std::min(native->signature().args().size(), names_.size());

  assert(values_.size() >= argc && "Argument count mismatch.");

  // printf("reorder: argc=%zu, names#=%zu\n", argc, names_.size());
  for (size_t i = 0; i != argc; ++i) {
    const std::string& localName = names_[i];
    const std::string& otherName = native->getNameAt(i);
    int nativeIndex = native->find(localName);

    // printf("reorder: localName=%s, otherName=%s, nindex=%i\n",
    // localName.c_str(), otherName.c_str(), nativeIndex);
    // ASTPrinter::print(values_[i]);

    if (static_cast<size_t>(nativeIndex) == i) {
      // OK: argument at correct position
      continue;
    }

    if (nativeIndex == -1) {
      int other = find(otherName);
      if (other != -1) {
        // OK: found expected arg at [other].
        swap(i, other);
      } else {
        superfluous->push_back(localName);
      }
      continue;
    }

    if (localName != otherName) {
      int other = find(otherName);
      assert(other != -1);
      swap(i, other);
    }
  }

  if (argc < names_.size()) {
    for (size_t i = argc, e = names_.size(); i != e; ++i) {
      superfluous->push_back(names_[i]);
    }
  }

  // dump("ParamList::reorder (after)");
}

void ParamList::dump(const char* title) {
  if (title && *title) {
    printf("%s\n", title);
  }
  for (int i = 0, e = size(); i != e; ++i) {
    printf("%16s: ", names_[i].c_str());
    ASTPrinter::print(values_[i]);
  }
}

int ParamList::find(const std::string& name) const {
  for (int i = 0, e = names_.size(); i != e; ++i) {
    if (names_[i] == name) return i;
  }
  return -1;
}

FlowLocation ParamList::location() const {
  if (values_.empty()) return FlowLocation();

  return FlowLocation(front()->location().filename, front()->location().begin,
                      back()->location().end);
}
// }}}

void Variable::visit(ASTVisitor& v) { v.accept(*this); }

Handler* Unit::findHandler(const std::string& name) {
  if (class Handler* handler =
          dynamic_cast<class Handler*>(scope()->lookup(name, Lookup::Self)))
    return handler;

  return nullptr;
}

void Unit::visit(ASTVisitor& v) { v.accept(*this); }

void Handler::visit(ASTVisitor& v) { v.accept(*this); }

void UnaryExpr::visit(ASTVisitor& v) { v.accept(*this); }

BinaryExpr::BinaryExpr(vm::Opcode op, std::unique_ptr<Expr>&& lhs,
                       std::unique_ptr<Expr>&& rhs)
    : Expr(rhs->location() - lhs->location()),
      operator_(op),
      lhs_(std::move(lhs)),
      rhs_(std::move(rhs)) {}

ArrayExpr::ArrayExpr(FlowLocation& loc,
                     std::vector<std::unique_ptr<Expr>>&& values)
    : Expr(loc), values_(std::move(values)) {}

ArrayExpr::~ArrayExpr() {}

void BinaryExpr::visit(ASTVisitor& v) { v.accept(*this); }

void ArrayExpr::visit(ASTVisitor& v) { v.accept(*this); }

void HandlerRefExpr::visit(ASTVisitor& v) { v.accept(*this); }

void CompoundStmt::push_back(std::unique_ptr<Stmt>&& stmt) {
  location_.update(stmt->location().end);
  statements_.push_back(std::move(stmt));
}

void CompoundStmt::visit(ASTVisitor& v) { v.accept(*this); }

void ExprStmt::visit(ASTVisitor& v) { v.accept(*this); }

void CallExpr::visit(ASTVisitor& v) { v.accept(*this); }

void BuiltinFunction::visit(ASTVisitor& v) { v.accept(*this); }

void VariableExpr::visit(ASTVisitor& v) { v.accept(*this); }

void CondStmt::visit(ASTVisitor& v) { v.accept(*this); }

void AssignStmt::visit(ASTVisitor& v) { v.accept(*this); }

void BuiltinHandler::visit(ASTVisitor& v) { v.accept(*this); }

void Handler::implement(std::unique_ptr<SymbolTable>&& table,
                        std::unique_ptr<Stmt>&& body) {
  scope_ = std::move(table);
  body_ = std::move(body);
}

bool CallExpr::setArgs(ParamList&& args) {
  args_ = std::move(args);
  if (!args_.empty()) {
    location().update(args_.back()->location().end);
  }

  return true;
}

MatchStmt::MatchStmt(const FlowLocation& loc, std::unique_ptr<Expr>&& cond,
                     vm::MatchClass op, std::list<MatchCase>&& cases,
                     std::unique_ptr<Stmt>&& elseStmt)
    : Stmt(loc),
      cond_(std::move(cond)),
      op_(op),
      cases_(std::move(cases)),
      elseStmt_(std::move(elseStmt)) {}

MatchStmt::MatchStmt(MatchStmt&& other)
    : Stmt(other.location()),
      cond_(std::move(other.cond_)),
      op_(std::move(other.op_)),
      cases_(std::move(other.cases_)) {}

MatchStmt& MatchStmt::operator=(MatchStmt&& other) {
  setLocation(other.location());
  cond_ = std::move(other.cond_);
  op_ = std::move(other.op_);
  cases_ = std::move(other.cases_);

  return *this;
}

MatchStmt::~MatchStmt() {}

void MatchStmt::visit(ASTVisitor& v) { v.accept(*this); }

// {{{ type system
FlowType UnaryExpr::getType() const { return resultType(op()); }

FlowType BinaryExpr::getType() const { return resultType(op()); }

FlowType ArrayExpr::getType() const {
  switch (values_.front()->getType()) {
    case FlowType::Number:
      return FlowType::IntArray;
    case FlowType::String:
      return FlowType::StringArray;
    case FlowType::IPAddress:
      return FlowType::IPAddrArray;
    case FlowType::Cidr:
      return FlowType::CidrArray;
    default:
      abort();
      return FlowType::Void;  // XXX error
  }
}

template <>
FLOW_API FlowType LiteralExpr<RegExp>::getType() const {
  return FlowType::RegExp;
}

template <>
FLOW_API FlowType LiteralExpr<Cidr>::getType() const {
  return FlowType::Cidr;
}

template <>
FLOW_API FlowType LiteralExpr<bool>::getType() const {
  return FlowType::Boolean;
}

template <>
FLOW_API FlowType LiteralExpr<IPAddress>::getType() const {
  return FlowType::IPAddress;
}

template <>
FLOW_API FlowType LiteralExpr<long long>::getType() const {
  return FlowType::Number;
}

template <>
FLOW_API FlowType LiteralExpr<std::string>::getType() const {
  return FlowType::String;
}

FlowType CallExpr::getType() const { return callee_->signature().returnType(); }

FlowType VariableExpr::getType() const {
  return variable_->initializer()->getType();
}

FlowType HandlerRefExpr::getType() const { return FlowType::Handler; }
// }}}

}  // namespace flow
}  // namespace xzero
