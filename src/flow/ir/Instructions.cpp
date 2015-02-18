// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/ir/BasicBlock.h>
#include <xzero/flow/ir/Instructions.h>
#include <xzero/flow/ir/InstructionVisitor.h>
#include <xzero/flow/ir/IRBuiltinHandler.h>
#include <xzero/flow/ir/IRBuiltinFunction.h>
#include <utility>  // make_pair
#include <assert.h>

namespace xzero {
namespace flow {

using namespace vm;

template <typename T, typename U>
inline std::vector<U> join(const T& a, const std::vector<U>& vec)  // {{{
{
  std::vector<U> res;

  res.push_back(a);
  for (const U& v : vec) res.push_back(v);

  return std::move(res);
}
// }}}
const char* cstr(UnaryOperator op)  // {{{
{
  static const char* ops[] = {
          // numerical
          [(size_t)UnaryOperator::INeg] = "ineg",
          [(size_t)UnaryOperator::INot] = "inot",
          // binary
          [(size_t)UnaryOperator::BNot] = "bnot",
          // string
          [(size_t)UnaryOperator::SLen] = "slen",
          [(size_t)UnaryOperator::SIsEmpty] = "sisempty", };

  return ops[(size_t)op];
}
// }}}
const char* cstr(BinaryOperator op)  // {{{
{
  static const char* ops[] = {
          // numerical
          [(size_t)BinaryOperator::IAdd] = "iadd",
          [(size_t)BinaryOperator::ISub] = "isub",
          [(size_t)BinaryOperator::IMul] = "imul",
          [(size_t)BinaryOperator::IDiv] = "idiv",
          [(size_t)BinaryOperator::IRem] = "irem",
          [(size_t)BinaryOperator::IPow] = "ipow",
          [(size_t)BinaryOperator::IAnd] = "iand",
          [(size_t)BinaryOperator::IOr] = "ior",
          [(size_t)BinaryOperator::IXor] = "ixor",
          [(size_t)BinaryOperator::IShl] = "ishl",
          [(size_t)BinaryOperator::IShr] = "ishr",
          [(size_t)BinaryOperator::ICmpEQ] = "icmpeq",
          [(size_t)BinaryOperator::ICmpNE] = "icmpne",
          [(size_t)BinaryOperator::ICmpLE] = "icmple",
          [(size_t)BinaryOperator::ICmpGE] = "icmpge",
          [(size_t)BinaryOperator::ICmpLT] = "icmplt",
          [(size_t)BinaryOperator::ICmpGT] = "icmpgt",
          // boolean
          [(size_t)BinaryOperator::BAnd] = "band",
          [(size_t)BinaryOperator::BOr] = "bor",
          [(size_t)BinaryOperator::BXor] = "bxor",
          // string
          [(size_t)BinaryOperator::SAdd] = "sadd",
          [(size_t)BinaryOperator::SSubStr] = "ssubstr",
          [(size_t)BinaryOperator::SCmpEQ] = "scmpeq",
          [(size_t)BinaryOperator::SCmpNE] = "scmpne",
          [(size_t)BinaryOperator::SCmpLE] = "scmple",
          [(size_t)BinaryOperator::SCmpGE] = "scmpge",
          [(size_t)BinaryOperator::SCmpLT] = "scmplt",
          [(size_t)BinaryOperator::SCmpGT] = "scmpgt",
          [(size_t)BinaryOperator::SCmpRE] = "scmpre",
          [(size_t)BinaryOperator::SCmpBeg] = "scmpbeg",
          [(size_t)BinaryOperator::SCmpEnd] = "scmpend",
          [(size_t)BinaryOperator::SIn] = "sin",
          // ip
          [(size_t)BinaryOperator::PCmpEQ] = "pcmpeq",
          [(size_t)BinaryOperator::PCmpNE] = "pcmpne",
          [(size_t)BinaryOperator::PInCidr] = "pincidr", };

  return ops[(size_t)op];
}
// }}}
// {{{ NopInstr
void NopInstr::dump() { dumpOne("NOP"); }

Instr* NopInstr::clone() { return new NopInstr(); }

void NopInstr::accept(InstructionVisitor& v) { v.visit(*this); }
// }}}
// {{{ CastInstr
void CastInstr::dump() { dumpOne(tos(type()).c_str()); }

Instr* CastInstr::clone() { return new CastInstr(type(), source(), name()); }

void CastInstr::accept(InstructionVisitor& v) { v.visit(*this); }
// }}}
// {{{ CondBrInstr
CondBrInstr::CondBrInstr(Value* cond, BasicBlock* trueBlock,
                         BasicBlock* falseBlock)
    : TerminateInstr({cond, trueBlock, falseBlock}) {}

void CondBrInstr::dump() { dumpOne("condbr"); }

Instr* CondBrInstr::clone() {
  return new CondBrInstr(condition(), trueBlock(), falseBlock());
}

void CondBrInstr::accept(InstructionVisitor& visitor) { visitor.visit(*this); }
// }}}
// {{{ BrInstr
BrInstr::BrInstr(BasicBlock* targetBlock) : TerminateInstr({targetBlock}) {}

void BrInstr::dump() { dumpOne("br"); }

Instr* BrInstr::clone() { return new BrInstr(targetBlock()); }

void BrInstr::accept(InstructionVisitor& visitor) { visitor.visit(*this); }
// }}}
// {{{ MatchInstr
MatchInstr::MatchInstr(MatchClass op, Value* cond)
    : TerminateInstr({cond, nullptr}), op_(op) {}

void MatchInstr::addCase(Constant* label, BasicBlock* code) {
  addOperand(label);
  addOperand(code);
}

void MatchInstr::setElseBlock(BasicBlock* code) { setOperand(1, code); }

BasicBlock* MatchInstr::elseBlock() const {
  return static_cast<BasicBlock*>(operand(1));
}

void MatchInstr::dump() {
  switch (op()) {
    case MatchClass::Same:
      dumpOne("match.same");
      break;
    case MatchClass::Head:
      dumpOne("match.head");
      break;
    case MatchClass::Tail:
      dumpOne("match.tail");
      break;
    case MatchClass::RegExp:
      dumpOne("match.re");
      break;
  }
}

MatchInstr::MatchInstr(const MatchInstr& v) : TerminateInstr(v), op_(v.op()) {}

Instr* MatchInstr::clone() { return new MatchInstr(*this); }

std::vector<std::pair<Constant*, BasicBlock*>> MatchInstr::cases() const {
  std::vector<std::pair<Constant*, BasicBlock*>> out;

  size_t caseCount = (operands().size() - 2) / 2;

  for (size_t i = 0; i < caseCount; ++i) {
    Constant* label = static_cast<Constant*>(operand(2 + 2 * i + 0));
    BasicBlock* code = static_cast<BasicBlock*>(operand(2 + 2 * i + 1));

    out.push_back(std::make_pair(label, code));
  }

  return out;
}

void MatchInstr::accept(InstructionVisitor& visitor) { visitor.visit(*this); }
// }}}
// {{{ RetInstr
RetInstr::RetInstr(Value* result) : TerminateInstr({result}) {}

void RetInstr::dump() { dumpOne("ret"); }

Instr* RetInstr::clone() { return new RetInstr(operand(0)); }

void RetInstr::accept(InstructionVisitor& visitor) { visitor.visit(*this); }
// }}}
// {{{ CallInstr
CallInstr::CallInstr(const std::vector<Value*>& args, const std::string& name)
    : Instr(static_cast<IRBuiltinFunction*>(args[0])->signature().returnType(),
            args, name) {}

CallInstr::CallInstr(IRBuiltinFunction* callee, const std::vector<Value*>& args,
                     const std::string& name)
    : Instr(callee->signature().returnType(), join(callee, args), name) {}

void CallInstr::dump() { dumpOne("call"); }

Instr* CallInstr::clone() { return new CallInstr(operands(), name()); }

void CallInstr::accept(InstructionVisitor& visitor) { visitor.visit(*this); }
// }}}
// {{{ HandlerCallInstr
HandlerCallInstr::HandlerCallInstr(const std::vector<Value*>& args)
    : Instr(FlowType::Void, args, "") {}

HandlerCallInstr::HandlerCallInstr(IRBuiltinHandler* callee,
                                   const std::vector<Value*>& args)
    : Instr(FlowType::Void, join(callee, args), "") {
  // XXX a handler call actually returns a boolean, but that's never used except
  // by the execution engine.
}

void HandlerCallInstr::dump() { dumpOne("handler"); }

Instr* HandlerCallInstr::clone() { return new HandlerCallInstr(operands()); }

void HandlerCallInstr::accept(InstructionVisitor& visitor) {
  visitor.visit(*this);
}
// }}}
// {{{ PhiNode
PhiNode::PhiNode(const std::vector<Value*>& ops, const std::string& name)
    : Instr(ops[0]->type(), ops, name) {}

void PhiNode::dump() { dumpOne("phi"); }

Instr* PhiNode::clone() { return new PhiNode(operands(), name()); }

void PhiNode::accept(InstructionVisitor& visitor) { visitor.visit(*this); }
// }}}
// {{{ other instructions
void AllocaInstr::accept(InstructionVisitor& visitor) { visitor.visit(*this); }

void StoreInstr::accept(InstructionVisitor& visitor) { visitor.visit(*this); }

void LoadInstr::accept(InstructionVisitor& visitor) { visitor.visit(*this); }

void AllocaInstr::dump() { dumpOne("alloca"); }

Instr* AllocaInstr::clone() {
  return new AllocaInstr(type(), operand(0), name());
}

void LoadInstr::dump() { dumpOne("load"); }

Instr* LoadInstr::clone() { return new LoadInstr(variable(), name()); }

void StoreInstr::dump() { dumpOne("store"); }

Instr* StoreInstr::clone() {
  return new StoreInstr(variable(), index(), expression(), name());
}
// }}}

}  // namespace flow
}  // namespace xzero
