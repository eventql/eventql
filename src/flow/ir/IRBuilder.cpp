// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/ir/IRBuilder.h>
#include <xzero/flow/ir/IRHandler.h>
#include <xzero/flow/ir/BasicBlock.h>
#include <xzero/flow/ir/ConstantValue.h>
#include <xzero/flow/ir/Instructions.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

namespace xzero {
namespace flow {

using namespace vm;

//#define FLOW_DEBUG_IR 1

#if defined(FLOW_DEBUG_IR)
#define TRACE(level, msg...) XZERO_DEBUG("IR", (level), msg)
#else
#define TRACE(level, msg...) do {} while (0)
#endif

IRBuilder::IRBuilder()
    : program_(nullptr),
      handler_(nullptr),
      insertPoint_(nullptr),
      nameStore_() {}

IRBuilder::~IRBuilder() {}

// {{{ name management
std::string IRBuilder::makeName(const std::string& name) {
  const std::string theName = name.empty() ? "tmp" : name;

  auto i = nameStore_.find(theName);
  if (i == nameStore_.end()) {
    nameStore_[theName] = 0;
    return theName;
  }

  unsigned long id = ++i->second;

  char buf[512];
  snprintf(buf, sizeof(buf), "%s%lu", theName.c_str(), id);
  return buf;
}
// }}}
// {{{ context management
void IRBuilder::setProgram(IRProgram* prog) {
  program_ = prog;
  handler_ = nullptr;
  insertPoint_ = nullptr;
}

IRHandler* IRBuilder::setHandler(IRHandler* hn) {
  assert(hn->parent() == program_);

  handler_ = hn;
  insertPoint_ = nullptr;

  return hn;
}

BasicBlock* IRBuilder::createBlock(const std::string& name) {
  std::string n = makeName(name);

  TRACE(1, "createBlock() %s", n.c_str());
  BasicBlock* bb = new BasicBlock(n);

  bb->setParent(handler_);
  handler_->blocks_.push_back(bb);

  return bb;
}

void IRBuilder::setInsertPoint(BasicBlock* bb) {
  assert(bb != nullptr);
  assert(bb->parent() == handler() &&
         "insert point must belong to the current handler.");

  TRACE(1, "setInsertPoint() %s", bb->name().c_str());

  insertPoint_ = bb;
  bb->parent_ = handler_;
}

Instr* IRBuilder::insert(Instr* instr) {
  assert(getInsertPoint() != nullptr);

  getInsertPoint()->push_back(instr);

  return instr;
}
// }}}
// {{{ handler pool
IRHandler* IRBuilder::getHandler(const std::string& name) {
  for (auto& item : program_->handlers_) {
    if (item->name() == name) return item;
  }

  IRHandler* h = new IRHandler(name);

  h->setParent(program_);
  program_->handlers_.push_back(h);

  return h;
}
// }}}
// {{{ value management
/**
 * dynamically allocates an array of given element type and size.
 */
AllocaInstr* IRBuilder::createAlloca(FlowType ty, Value* arraySize,
                                     const std::string& name) {
  return static_cast<AllocaInstr*>(
      insert(new AllocaInstr(ty, arraySize, makeName(name))));
}

/**
 * Loads given value
 */
Value* IRBuilder::createLoad(Value* value, const std::string& name) {
  if (dynamic_cast<Constant*>(value)) return value;

  // if (dynamic_cast<Variable*>(value))
  return insert(new LoadInstr(value, makeName(name)));

  assert(!"Value must be of type Constant or Variable.");
  return nullptr;
}

/**
 * emits a STORE of value \p rhs to variable \p lhs.
 */
Instr* IRBuilder::createStore(Value* lhs, Value* rhs, const std::string& name) {
  return createStore(lhs, get(0), rhs, name);
}

Instr* IRBuilder::createStore(Value* lhs, ConstantInt* index, Value* rhs,
                              const std::string& name) {
  assert(dynamic_cast<AllocaInstr*>(lhs) &&
         "lhs must be of type AllocaInstr in order to STORE to.");
  // assert(lhs->type() == rhs->type() && "Type of lhs and rhs must be equal.");
  // assert(dynamic_cast<IRVariable*>(lhs) && "lhs must be of type Variable.");

  return insert(new StoreInstr(lhs, index, rhs, makeName(name)));
}

Instr* IRBuilder::createPhi(const std::vector<Value*>& incomings,
                            const std::string& name) {
  return insert(new PhiNode(incomings, makeName(name)));
}
// }}}
// {{{ boolean ops
Value* IRBuilder::createBNot(Value* rhs, const std::string& name) {
  assert(rhs->type() == FlowType::Boolean);

  if (auto a = dynamic_cast<ConstantBoolean*>(rhs))
    return getBoolean(!a->get());

  return insert(new BNotInstr(rhs, makeName(name)));
}

Value* IRBuilder::createBAnd(Value* lhs, Value* rhs, const std::string& name) {
  assert(rhs->type() == FlowType::Boolean);

  if (auto a = dynamic_cast<ConstantBoolean*>(lhs))
    if (auto b = dynamic_cast<ConstantBoolean*>(rhs))
      return getBoolean(a->get() && b->get());

  return insert(new BAndInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createBXor(Value* lhs, Value* rhs, const std::string& name) {
  assert(rhs->type() == FlowType::Boolean);

  if (auto a = dynamic_cast<ConstantBoolean*>(lhs))
    if (auto b = dynamic_cast<ConstantBoolean*>(rhs))
      return getBoolean(a->get() ^ b->get());

  return insert(new BAndInstr(lhs, rhs, makeName(name)));
}
// }}}
// {{{ numerical ops
Value* IRBuilder::createNeg(Value* rhs, const std::string& name) {
  assert(rhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(rhs)) return get(-a->get());

  return insert(new INegInstr(rhs, makeName(name)));
}

Value* IRBuilder::createNot(Value* rhs, const std::string& name) {
  assert(rhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(rhs)) return get(~a->get());

  return insert(new INotInstr(rhs, makeName(name)));
}

Value* IRBuilder::createAdd(Value* lhs, Value* rhs, const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() + b->get());

  return insert(new IAddInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createSub(Value* lhs, Value* rhs, const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() - b->get());

  return insert(new ISubInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createMul(Value* lhs, Value* rhs, const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() * b->get());

  return insert(new IMulInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createDiv(Value* lhs, Value* rhs, const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() / b->get());

  return insert(new IDivInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createRem(Value* lhs, Value* rhs, const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() % b->get());

  return insert(new IRemInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createShl(Value* lhs, Value* rhs, const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() << b->get());

  return insert(new IShlInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createShr(Value* lhs, Value* rhs, const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() >> b->get());

  return insert(new IShrInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createPow(Value* lhs, Value* rhs, const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(powl(a->get(), b->get()));

  return insert(new IPowInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createAnd(Value* lhs, Value* rhs, const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() & b->get());

  return insert(new IAndInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createOr(Value* lhs, Value* rhs, const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() | b->get());

  return insert(new IOrInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createXor(Value* lhs, Value* rhs, const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() ^ b->get());

  return insert(new IXorInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createNCmpEQ(Value* lhs, Value* rhs,
                               const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() == b->get());

  return insert(new ICmpEQInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createNCmpNE(Value* lhs, Value* rhs,
                               const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() != b->get());

  return insert(new ICmpNEInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createNCmpLE(Value* lhs, Value* rhs,
                               const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() <= b->get());

  return insert(new ICmpLEInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createNCmpGE(Value* lhs, Value* rhs,
                               const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() >= b->get());

  return insert(new ICmpGEInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createNCmpLT(Value* lhs, Value* rhs,
                               const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() < b->get());

  return insert(new ICmpLTInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createNCmpGT(Value* lhs, Value* rhs,
                               const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::Number);

  if (auto a = dynamic_cast<ConstantInt*>(lhs))
    if (auto b = dynamic_cast<ConstantInt*>(rhs))
      return get(a->get() > b->get());

  return insert(new ICmpGTInstr(lhs, rhs, makeName(name)));
}
// }}}
// {{{ string ops
Value* IRBuilder::createSAdd(Value* lhs, Value* rhs, const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::String);

  if (auto a = dynamic_cast<ConstantString*>(lhs)) {
    if (auto b = dynamic_cast<ConstantString*>(rhs)) {
      return get(a->get() + b->get());
    }

    if (a->get().empty()) {
      return rhs;
    }
  } else if (auto b = dynamic_cast<ConstantString*>(rhs)) {
    if (b->get().empty()) {
      return rhs;
    }
  }

  return insert(new SAddInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createSCmpEQ(Value* lhs, Value* rhs,
                               const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::String);

  if (auto a = dynamic_cast<ConstantString*>(lhs))
    if (auto b = dynamic_cast<ConstantString*>(rhs))
      return get(a->get() == b->get());

  return insert(new SCmpEQInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createSCmpNE(Value* lhs, Value* rhs,
                               const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::String);

  if (auto a = dynamic_cast<ConstantString*>(lhs))
    if (auto b = dynamic_cast<ConstantString*>(rhs))
      return get(a->get() != b->get());

  return insert(new SCmpNEInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createSCmpLE(Value* lhs, Value* rhs,
                               const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::String);

  if (auto a = dynamic_cast<ConstantString*>(lhs))
    if (auto b = dynamic_cast<ConstantString*>(rhs))
      return get(a->get() <= b->get());

  return insert(new SCmpLEInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createSCmpGE(Value* lhs, Value* rhs,
                               const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::String);

  if (auto a = dynamic_cast<ConstantString*>(lhs))
    if (auto b = dynamic_cast<ConstantString*>(rhs))
      return get(a->get() >= b->get());

  return insert(new SCmpGEInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createSCmpLT(Value* lhs, Value* rhs,
                               const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::String);

  if (auto a = dynamic_cast<ConstantString*>(lhs))
    if (auto b = dynamic_cast<ConstantString*>(rhs))
      return get(a->get() < b->get());

  return insert(new SCmpLTInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createSCmpGT(Value* lhs, Value* rhs,
                               const std::string& name) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type() == FlowType::String);

  if (auto a = dynamic_cast<ConstantString*>(lhs))
    if (auto b = dynamic_cast<ConstantString*>(rhs))
      return get(a->get() > b->get());

  return insert(new SCmpGTInstr(lhs, rhs, makeName(name)));
}

/**
 * Compare string \p lhs against regexp \p rhs.
 */
Value* IRBuilder::createSCmpRE(Value* lhs, Value* rhs,
                               const std::string& name) {
  assert(lhs->type() == FlowType::String);
  assert(rhs->type() == FlowType::RegExp);

  // XXX don't perform constant folding on (string =~ regexp) as this operation
  // yields side affects to: regex.group(I)S

  return insert(new SCmpREInstr(lhs, rhs, makeName(name)));
}

/**
 * Tests if string \p lhs begins with string \p rhs.
 *
 * @param lhs test string
 * @param rhs sub string needle
 * @param name Name of the given operations result value.
 *
 * @return boolean result.
 */
Value* IRBuilder::createSCmpEB(Value* lhs, Value* rhs,
                               const std::string& name) {
  if (auto a = dynamic_cast<ConstantString*>(lhs))
    if (auto b = dynamic_cast<ConstantString*>(rhs))
      return get(BufferRef(a->get()).begins(b->get()));

  return insert(new SCmpBegInstr(lhs, rhs, makeName(name)));
}

/**
 * Tests if string \p lhs ends with string \p rhs.
 *
 * @param lhs test string
 * @param rhs sub string needle
 * @param name Name of the given operations result value.
 *
 * @return boolean result.
 */
Value* IRBuilder::createSCmpEE(Value* lhs, Value* rhs,
                               const std::string& name) {
  if (auto a = dynamic_cast<ConstantString*>(lhs))
    if (auto b = dynamic_cast<ConstantString*>(rhs))
      return get(BufferRef(a->get()).ends(b->get()));

  return insert(new SCmpEndInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createSIn(Value* lhs, Value* rhs, const std::string& name) {
  if (auto a = dynamic_cast<ConstantString*>(lhs))
    if (auto b = dynamic_cast<ConstantString*>(rhs))
      return get(BufferRef(a->get()).find(b->get()) != Buffer::npos);

  return insert(new SInInstr(lhs, rhs, makeName(name)));
}
// }}}
// {{{ ip ops
Value* IRBuilder::createPCmpEQ(Value* lhs, Value* rhs,
                               const std::string& name) {
  if (auto a = dynamic_cast<ConstantIP*>(lhs))
    if (auto b = dynamic_cast<ConstantIP*>(rhs))
      return get(a->get() == b->get());

  return insert(new PCmpEQInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createPCmpNE(Value* lhs, Value* rhs,
                               const std::string& name) {
  if (auto a = dynamic_cast<ConstantIP*>(lhs))
    if (auto b = dynamic_cast<ConstantIP*>(rhs))
      return get(a->get() != b->get());

  return insert(new PCmpNEInstr(lhs, rhs, makeName(name)));
}

Value* IRBuilder::createPInCidr(Value* lhs, Value* rhs,
                                const std::string& name) {
  if (auto a = dynamic_cast<ConstantIP*>(lhs))
    if (auto b = dynamic_cast<ConstantCidr*>(rhs))
      return get(b->get().contains(a->get()));

  return insert(new PInCidrInstr(lhs, rhs, makeName(name)));
}
// }}}
// {{{ cast ops
Value* IRBuilder::createB2S(Value* rhs, const std::string& name) {
  assert(rhs->type() == FlowType::Boolean);

  if (auto a = dynamic_cast<ConstantBoolean*>(rhs))
    return get(a->get() ? "true" : "false");

  return insert(new CastInstr(FlowType::String, rhs, makeName(name)));
}

Value* IRBuilder::createI2S(Value* rhs, const std::string& name) {
  assert(rhs->type() == FlowType::Number);

  if (auto i = dynamic_cast<ConstantInt*>(rhs)) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%" PRIi64 "", i->get());
    return get(buf);
  }

  return insert(new CastInstr(FlowType::String, rhs, makeName(name)));
}

Value* IRBuilder::createP2S(Value* rhs, const std::string& name) {
  assert(rhs->type() == FlowType::IPAddress);

  if (auto ip = dynamic_cast<ConstantIP*>(rhs)) return get(ip->get().str());

  return insert(new CastInstr(FlowType::String, rhs, makeName(name)));
}

Value* IRBuilder::createC2S(Value* rhs, const std::string& name) {
  assert(rhs->type() == FlowType::Cidr);

  if (auto ip = dynamic_cast<ConstantCidr*>(rhs)) return get(ip->get().str());

  return insert(new CastInstr(FlowType::String, rhs, makeName(name)));
}

Value* IRBuilder::createR2S(Value* rhs, const std::string& name) {
  assert(rhs->type() == FlowType::RegExp);

  if (auto ip = dynamic_cast<ConstantRegExp*>(rhs))
    return get(ip->get().pattern());

  return insert(new CastInstr(FlowType::String, rhs, makeName(name)));
}

Value* IRBuilder::createS2I(Value* rhs, const std::string& name) {
  assert(rhs->type() == FlowType::String);

  if (auto ip = dynamic_cast<ConstantString*>(rhs)) return get(stoi(ip->get()));

  return insert(new CastInstr(FlowType::Number, rhs, makeName(name)));
}
// }}}
// {{{ call creators
Instr* IRBuilder::createCallFunction(IRBuiltinFunction* callee,
                                     const std::vector<Value*>& args,
                                     const std::string& name) {
  return insert(new CallInstr(callee, args, makeName(name)));
}

Instr* IRBuilder::createInvokeHandler(IRBuiltinHandler* callee,
                                      const std::vector<Value*>& args) {
  return insert(new HandlerCallInstr(callee, args));
}
// }}}
// {{{ exit point creators
Instr* IRBuilder::createRet(Value* result) {
  return insert(new RetInstr(result));
}

Instr* IRBuilder::createBr(BasicBlock* target) {
  return insert(new BrInstr(target));
}

Instr* IRBuilder::createCondBr(Value* condValue, BasicBlock* trueBlock,
                               BasicBlock* falseBlock) {
  return insert(new CondBrInstr(condValue, trueBlock, falseBlock));
}

MatchInstr* IRBuilder::createMatch(vm::MatchClass opc, Value* cond) {
  return static_cast<MatchInstr*>(insert(new MatchInstr(opc, cond)));
}

Value* IRBuilder::createMatchSame(Value* cond) {
  return createMatch(MatchClass::Same, cond);
}

Value* IRBuilder::createMatchHead(Value* cond) {
  return createMatch(MatchClass::Head, cond);
}

Value* IRBuilder::createMatchTail(Value* cond) {
  return createMatch(MatchClass::Tail, cond);
}

Value* IRBuilder::createMatchRegExp(Value* cond) {
  return createMatch(MatchClass::RegExp, cond);
}
// }}}

}  // namespace flow
}  // namespace xzero
