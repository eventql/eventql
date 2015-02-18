// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/TargetCodeGenerator.h>
#include <xzero/flow/vm/ConstantPool.h>
#include <xzero/flow/vm/Program.h>
#include <xzero/flow/ir/BasicBlock.h>
#include <xzero/flow/ir/ConstantValue.h>
#include <xzero/flow/ir/ConstantArray.h>
#include <xzero/flow/ir/Instructions.h>
#include <xzero/flow/ir/IRProgram.h>
#include <xzero/flow/ir/IRHandler.h>
#include <xzero/flow/ir/IRBuiltinHandler.h>
#include <xzero/flow/ir/IRBuiltinFunction.h>
#include <xzero/flow/FlowType.h>
#include <unordered_map>

namespace xzero {
namespace flow {

using namespace vm;

template <typename T, typename S>
std::vector<T> convert(const std::vector<Constant*>& source) {
  std::vector<T> target(source.size());

  for (size_t i = 0, e = source.size(); i != e; ++i)
    target[i] = static_cast<S*>(source[i])->get();

  return target;
}

TargetCodeGenerator::TargetCodeGenerator()
    : errors_(),
      conditionalJumps_(),
      unconditionalJumps_(),
      handlerId_(0),
      code_(),
      variables_(),
      allocations_() {
  // preserve r0, so it'll never be used.
  allocations_.push_back(true);
}

TargetCodeGenerator::~TargetCodeGenerator() {}

std::unique_ptr<vm::Program> TargetCodeGenerator::generate(
    IRProgram* program) {
  for (IRHandler* handler : program->handlers()) generate(handler);

  cp_.setModules(program->modules());

  return std::unique_ptr<vm::Program>(new vm::Program(std::move(cp_)));
}

void TargetCodeGenerator::generate(IRHandler* handler) {
  // explicitely forward-declare handler, so we can use its ID internally.
  handlerId_ = handlerRef(handler);

  std::unordered_map<BasicBlock*, size_t> basicBlockEntryPoints;

  // generate code for all basic blocks, sequentially
  for (BasicBlock* bb : handler->basicBlocks()) {
    basicBlockEntryPoints[bb] = getInstructionPointer();
    for (Instr* instr : bb->instructions()) {
      instr->accept(*this);
    }
  }

  // fixiate conditional jump instructions
  for (const auto& target : conditionalJumps_) {
    size_t targetPC = basicBlockEntryPoints[target.first];
    for (const auto& source : target.second) {
      code_[source.pc] =
          makeInstruction(source.opcode, source.condition, targetPC);
    }
  }
  conditionalJumps_.clear();

  // fixiate unconditional jump instructions
  for (const auto& target : unconditionalJumps_) {
    size_t targetPC = basicBlockEntryPoints[target.first];
    for (const auto& source : target.second) {
      code_[source.pc] = makeInstruction(source.opcode, targetPC);
    }
  }
  unconditionalJumps_.clear();

  // fixiate match jump table
  for (const auto& hint : matchHints_) {
    size_t matchId = hint.second;
    MatchInstr* instr = hint.first;
    const auto& cases = instr->cases();
    MatchDef& def = cp_.getMatchDef(matchId);

    for (size_t i = 0, e = cases.size(); i != e; ++i) {
      def.cases[i].pc = basicBlockEntryPoints[cases[i].second];
    }

    if (instr->elseBlock()) {
      def.elsePC = basicBlockEntryPoints[instr->elseBlock()];
    }
  }
  matchHints_.clear();

  cp_.getHandler(handlerId_).second = std::move(code_);

  // cleanup remaining handler-local work vars
  allocations_.clear();
  variables_.clear();
}

/**
 * Retrieves the program's handler ID for given handler, possibly
 * forward-declaring given handler if not (yet) found.
 */
size_t TargetCodeGenerator::handlerRef(IRHandler* handler) {
  return cp_.makeHandler(handler->name());
}

size_t TargetCodeGenerator::emit(vm::Instruction instr) {
  code_.push_back(instr);
  return getInstructionPointer() - 1;
}

size_t TargetCodeGenerator::emit(vm::Opcode opcode, Register cond,
                                 BasicBlock* bb) {
  size_t pc = emit(Opcode::NOP);
  conditionalJumps_[bb].push_back({pc, opcode, cond});
  return pc;
}

size_t TargetCodeGenerator::emit(vm::Opcode opcode, BasicBlock* bb) {
  size_t pc = emit(Opcode::NOP);
  unconditionalJumps_[bb].push_back({pc, opcode});
  return pc;
}

size_t TargetCodeGenerator::emitBinary(Instr& instr, Opcode rr) {
  assert(operandSignature(rr) == InstructionSig::RRR);

  Register a = allocate(1, instr);
  Register b = getRegister(instr.operand(0));
  Register c = getRegister(instr.operand(1));

  return emit(rr, a, b, c);
}

size_t TargetCodeGenerator::emitBinaryAssoc(Instr& instr, Opcode rr,
                                            Opcode ri) {
  assert(operandSignature(rr) == InstructionSig::RRR);
  assert(operandSignature(ri) == InstructionSig::RRI);

  Register a = allocate(1, instr);

  if (auto i = dynamic_cast<ConstantInt*>(instr.operand(1))) {
    Register b = getRegister(instr.operand(0));
    return emit(ri, a, b, i->get());
  }

  if (auto i = dynamic_cast<ConstantInt*>(instr.operand(0))) {
    Register b = getRegister(instr.operand(1));
    return emit(ri, a, b, i->get());
  }

  Register b = getRegister(instr.operand(0));
  Register c = getRegister(instr.operand(1));
  return emit(rr, a, b, c);
}

size_t TargetCodeGenerator::emitBinary(Instr& instr, Opcode rr, Opcode ri) {
  assert(operandSignature(rr) == InstructionSig::RRR);
  assert(operandSignature(ri) == InstructionSig::RRI);

  Register a = allocate(1, instr);

  if (auto i = dynamic_cast<ConstantInt*>(instr.operand(1))) {
    Register b = getRegister(instr.operand(0));
    return emit(ri, a, b, i->get());
  }

  Register b = getRegister(instr.operand(0));
  Register c = getRegister(instr.operand(1));
  return emit(rr, a, b, c);
}

size_t TargetCodeGenerator::emitUnary(Instr& instr, vm::Opcode r) {
  assert(operandSignature(r) == InstructionSig::RR);

  Register a = allocate(1, instr);
  Register b = getRegister(instr.operand(0));

  return emit(r, a, b);
}

size_t TargetCodeGenerator::allocate(size_t count, Value& alias) {
  int rbase = allocate(count);
  variables_[&alias] = rbase;
  return rbase;
}

size_t TargetCodeGenerator::allocate(size_t count) {
  for (size_t i = 0; i < count; ++i) allocations_.push_back(true);

  return allocations_.size() - count;
}

void TargetCodeGenerator::free(size_t base, size_t count) {
  for (int i = base, e = base + count; i != e; ++i) {
    allocations_[i] = false;
  }
}

// {{{ instruction code generation
void TargetCodeGenerator::visit(NopInstr& instr) { emit(Opcode::NOP); }

void TargetCodeGenerator::visit(AllocaInstr& instr) {
  allocate(getConstantInt(instr.arraySize()), instr);
}

size_t TargetCodeGenerator::makeNumber(FlowNumber value) {
  return cp_.makeInteger(value);
}

size_t TargetCodeGenerator::makeNativeHandler(IRBuiltinHandler* builtin) {
  return cp_.makeNativeHandler(builtin->signature().to_s());
}

size_t TargetCodeGenerator::makeNativeFunction(IRBuiltinFunction* builtin) {
  return cp_.makeNativeFunction(builtin->signature().to_s());
}

void TargetCodeGenerator::visit(StoreInstr& instr) {
  Value* lhs = instr.variable();
  size_t index = getConstantInt(instr.index());
  Value* rhs = instr.expression();

  Register lhsReg = getRegister(lhs) + index;

  // const int
  if (auto integer = dynamic_cast<ConstantInt*>(rhs)) {
    FlowNumber number = integer->get();
    if (number == int16_t(number)) {  // limit to 16bit signed width
      emit(Opcode::IMOV, lhsReg, number);
    } else {
      emit(Opcode::NCONST, lhsReg, cp_.makeInteger(number));
    }
    return;
  }

  // const boolean
  if (auto boolean = dynamic_cast<ConstantBoolean*>(rhs)) {
    emit(Opcode::IMOV, lhsReg, boolean->get());
    return;
  }

  // const string
  if (auto string = dynamic_cast<ConstantString*>(rhs)) {
    emit(Opcode::SCONST, lhsReg, cp_.makeString(string->get()));
    return;
  }

  // const IP address
  if (auto ip = dynamic_cast<ConstantIP*>(rhs)) {
    emit(Opcode::PCONST, lhsReg, cp_.makeIPAddress(ip->get()));
    return;
  }

  // const Cidr
  if (auto cidr = dynamic_cast<ConstantCidr*>(rhs)) {
    emit(Opcode::CCONST, lhsReg, cp_.makeCidr(cidr->get()));
    return;
  }

  // TODO const RegExp
  if (/*auto re =*/dynamic_cast<ConstantRegExp*>(rhs)) {
    assert(!"TODO store const RegExp");
    return;
  }

  if (auto array = dynamic_cast<ConstantArray*>(rhs)) {
    switch (array->type()) {
      case FlowType::IntArray:
        emit(Opcode::ITCONST, lhsReg,
             cp_.makeIntegerArray(
                 convert<FlowNumber, ConstantInt>(array->get())));
        return;
      case FlowType::StringArray:
        emit(Opcode::STCONST, lhsReg,
             cp_.makeStringArray(
                 convert<std::string, ConstantString>(array->get())));
        return;
      case FlowType::IPAddrArray:
        emit(Opcode::PTCONST, lhsReg,
             cp_.makeIPaddrArray(convert<IPAddress, ConstantIP>(array->get())));
        return;
      case FlowType::CidrArray:
        emit(Opcode::CTCONST, lhsReg,
             cp_.makeCidrArray(convert<Cidr, ConstantCidr>(array->get())));
        return;
      default:
        assert(!"BUG: array");
        abort();
    }
  }

  // var = var
  auto i = variables_.find(rhs);
  if (i != variables_.end()) {
    emit(Opcode::MOV, lhsReg, i->second);
    return;
  }

#ifndef NDEBUG
  printf("instr:\n");
  instr.dump();
  printf("lhs:\n");
  lhs->dump();
  printf("rhs:\n");
  rhs->dump();
#endif
  assert(!"Store variant not implemented!");
}

void TargetCodeGenerator::visit(LoadInstr& instr) {
  // no need to *load* the variable into a register as
  // we have only one variable store
  variables_[&instr] = getRegister(instr.variable());
}

Register TargetCodeGenerator::emitCallArgs(Instr& instr) {
  int argc = instr.operands().size();
  Register rbase = allocate(argc, instr);

  for (int i = 1; i < argc; ++i) {
    Register tmp = getRegister(instr.operands()[i]);
    if (auto alloca = dynamic_cast<AllocaInstr*>(instr.operands()[i])) {
      size_t n = getConstantInt(alloca->arraySize());
      if (n > 1) {
        emit(Opcode::IMOV, rbase + i, tmp);
        continue;
      }
    }
    emit(Opcode::MOV, rbase + i, tmp);
  }

  return rbase;
}

void TargetCodeGenerator::visit(CallInstr& instr) {
  int argc = instr.operands().size();

  Register rbase = emitCallArgs(instr);

  // emit call
  Register nativeId = makeNativeFunction(instr.callee());
  emit(Opcode::CALL, nativeId, argc, rbase);

  variables_[&instr] = rbase;

  free(rbase + 1, argc - 1);
}

void TargetCodeGenerator::visit(HandlerCallInstr& instr) {
  int argc = instr.operands().size();

  Register rbase = emitCallArgs(instr);

  // emit call
  Register nativeId = makeNativeHandler(instr.callee());
  emit(Opcode::HANDLER, nativeId, argc, rbase);

  variables_[&instr] = rbase;

  free(rbase + 1, argc - 1);
}

vm::Operand TargetCodeGenerator::getConstantInt(Value* value) {
  if (auto i = dynamic_cast<ConstantInt*>(value)) return i->get();

  assert(!"Should not happen");
  return 0;
}

vm::Operand TargetCodeGenerator::getRegister(Value* value) {
  auto i = variables_.find(value);
  if (i != variables_.end()) return i->second;

  // const int
  if (ConstantInt* integer = dynamic_cast<ConstantInt*>(value)) {
    // FIXME this constant initialization should pretty much be done in the
    // entry block
    Register reg = allocate(1);
    FlowNumber number = integer->get();
    if (number == int16_t(number)) {  // limit to 16bit signed width
      emit(Opcode::IMOV, reg, number);
    } else {
      emit(Opcode::NCONST, reg, cp_.makeInteger(number));
    }
    return reg;
  }

  // const boolean
  if (auto boolean = dynamic_cast<ConstantBoolean*>(value)) {
    Register reg = allocate(1);
    emit(Opcode::IMOV, reg, boolean->get());
    return reg;
  }

  // const string
  if (ConstantString* str = dynamic_cast<ConstantString*>(value)) {
    Register reg = allocate(1);
    emit(Opcode::SCONST, reg, cp_.makeString(str->get()));
    return reg;
  }

  // const ip
  if (ConstantIP* ip = dynamic_cast<ConstantIP*>(value)) {
    Register reg = allocate(1);
    emit(Opcode::PCONST, reg, cp_.makeIPAddress(ip->get()));
    return reg;
  }

  // const cidr
  if (ConstantCidr* cidr = dynamic_cast<ConstantCidr*>(value)) {
    Register reg = allocate(1);
    emit(Opcode::CCONST, reg, cp_.makeCidr(cidr->get()));
    return reg;
  }

  // const array<T>
  if (ConstantArray* array = dynamic_cast<ConstantArray*>(value)) {
    Register reg = allocate(1);
    switch (array->type()) {
      case FlowType::IntArray:
        emit(Opcode::ITCONST, reg,
             cp_.makeIntegerArray(
                 convert<FlowNumber, ConstantInt>(array->get())));
        break;
      case FlowType::StringArray:
        emit(Opcode::STCONST, reg,
             cp_.makeStringArray(
                 convert<std::string, ConstantString>(array->get())));
        break;
      case FlowType::IPAddrArray:
        emit(Opcode::PTCONST, reg,
             cp_.makeIPaddrArray(convert<IPAddress, ConstantIP>(array->get())));
        break;
      case FlowType::CidrArray:
        emit(Opcode::CTCONST, reg,
             cp_.makeCidrArray(convert<Cidr, ConstantCidr>(array->get())));
        break;
      default:
        assert(!"BUG: array");
        abort();
    }
    return reg;
  }

  // const regex
  if (/*ConstantRegExp* re =*/dynamic_cast<ConstantRegExp*>(value)) {
    Register reg = allocate(1);
    // emit(Opcode::RCONST, reg, re->id());
    assert(!"TODO: RCONST opcode");
    return reg;
  }

  return allocate(1, value);
}

void TargetCodeGenerator::visit(PhiNode& instr) {
  assert(!"Should never reach here, as PHI instruction nodes should have been replaced by target registers.");
}

void TargetCodeGenerator::visit(CondBrInstr& instr) {
  if (instr.parent()->isAfter(instr.trueBlock())) {
    emit(Opcode::JZ, getRegister(instr.condition()), instr.falseBlock());
  } else if (instr.parent()->isAfter(instr.falseBlock())) {
    emit(Opcode::JN, getRegister(instr.condition()), instr.trueBlock());
  } else {
    emit(Opcode::JN, getRegister(instr.condition()), instr.trueBlock());
    emit(Opcode::JMP, instr.falseBlock());
  }
}

void TargetCodeGenerator::visit(BrInstr& instr) {
  // to not emit the JMP if the target block is emitted right after this block
  // (and thus, right after this instruction).
  if (instr.parent()->isAfter(instr.targetBlock())) return;

  emit(Opcode::JMP, instr.targetBlock());
}

void TargetCodeGenerator::visit(RetInstr& instr) {
  emit(Opcode::EXIT, getConstantInt(instr.operands()[0]));
}

void TargetCodeGenerator::visit(MatchInstr& instr) {
  static const Opcode ops[] = {[(size_t)MatchClass::Same] = Opcode::SMATCHEQ,
                               [(size_t)MatchClass::Head] = Opcode::SMATCHBEG,
                               [(size_t)MatchClass::Tail] = Opcode::SMATCHEND,
                               [(size_t)MatchClass::RegExp] =
                                   Opcode::SMATCHR, };

  const size_t matchId = cp_.makeMatchDef();
  MatchDef& matchDef = cp_.getMatchDef(matchId);

  matchDef.handlerId = handlerRef(instr.parent()->parent());
  matchDef.op = instr.op();
  matchDef.elsePC = 0;  // XXX to be filled in post-processing the handler

  matchHints_.push_back({&instr, matchId});

  for (const auto& one : instr.cases()) {
    MatchCaseDef caseDef;
    switch (one.first->type()) {
      case FlowType::String:
        caseDef.label =
            cp_.makeString(static_cast<ConstantString*>(one.first)->get());
        break;
      case FlowType::RegExp:
        caseDef.label =
            cp_.makeRegExp(static_cast<ConstantRegExp*>(one.first)->get());
        break;
      default:
        assert(!"BUG: unsupported label type");
        abort();
    }
    caseDef.pc = 0;  // XXX to be filled in post-processing the handler

    matchDef.cases.push_back(caseDef);
  }

  Register condition = getRegister(instr.condition());

  emit(ops[(size_t)matchDef.op], condition, matchId);
}

void TargetCodeGenerator::visit(CastInstr& instr) {
  // map of (target, source, opcode)
  static const std::unordered_map<FlowType,
                                  std::unordered_map<FlowType, Opcode>> map =
      {{FlowType::String,
        {{FlowType::Number, Opcode::I2S},
         {FlowType::IPAddress, Opcode::P2S},
         {FlowType::Cidr, Opcode::C2S},
         {FlowType::RegExp, Opcode::R2S}, }},
       {FlowType::Number, {{FlowType::String, Opcode::I2S}, }}, };

  // just alias same-type casts
  if (instr.type() == instr.source()->type()) {
    variables_[&instr] = getRegister(instr.source());
    return;
  }

  // lookup target type
  const auto i = map.find(instr.type());
  assert(i != map.end() && "Cast target type not found.");

  // lookup source type
  const auto& sub = i->second;
  auto k = sub.find(instr.source()->type());
  assert(k != sub.end() && "Cast source type not found.");
  Opcode op = k->second;

  // emit instruction
  Register result = allocate(1, instr);
  Register a = getRegister(instr.source());
  emit(op, result, a);
}

void TargetCodeGenerator::visit(INegInstr& instr) {
  emitUnary(instr, Opcode::NNEG);
}

void TargetCodeGenerator::visit(INotInstr& instr) {
  Register a = allocate(1, instr);
  Register b = getRegister(instr.operands()[0]);
  emit(Opcode::NNOT, a, b);
}

void TargetCodeGenerator::visit(IAddInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NADD, Opcode::NIADD);
}

void TargetCodeGenerator::visit(ISubInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NSUB, Opcode::NISUB);
}

void TargetCodeGenerator::visit(IMulInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NMUL, Opcode::NIMUL);
}

void TargetCodeGenerator::visit(IDivInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NDIV, Opcode::NIDIV);
}

void TargetCodeGenerator::visit(IRemInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NREM, Opcode::NIREM);
}

void TargetCodeGenerator::visit(IPowInstr& instr) {
  emitBinary(instr, Opcode::NPOW, Opcode::NIPOW);
}

void TargetCodeGenerator::visit(IAndInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NAND, Opcode::NIAND);
}

void TargetCodeGenerator::visit(IOrInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NOR, Opcode::NIOR);
}

void TargetCodeGenerator::visit(IXorInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NXOR, Opcode::NIXOR);
}

void TargetCodeGenerator::visit(IShlInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NSHL, Opcode::NISHL);
}

void TargetCodeGenerator::visit(IShrInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NSHR, Opcode::NISHR);
}

void TargetCodeGenerator::visit(ICmpEQInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NCMPEQ, Opcode::NICMPEQ);
}

void TargetCodeGenerator::visit(ICmpNEInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NCMPNE, Opcode::NICMPNE);
}

void TargetCodeGenerator::visit(ICmpLEInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NCMPLE, Opcode::NICMPLE);
}

void TargetCodeGenerator::visit(ICmpGEInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NCMPGE, Opcode::NICMPGE);
}

void TargetCodeGenerator::visit(ICmpLTInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NCMPLT, Opcode::NICMPLT);
}

void TargetCodeGenerator::visit(ICmpGTInstr& instr) {
  emitBinaryAssoc(instr, Opcode::NCMPGT, Opcode::NICMPGT);
}

void TargetCodeGenerator::visit(BNotInstr& instr) {
  emitUnary(instr, Opcode::BNOT);
}

void TargetCodeGenerator::visit(BAndInstr& instr) {
  emitBinary(instr, Opcode::BAND);
}

void TargetCodeGenerator::visit(BOrInstr& instr) {
  emitBinary(instr, Opcode::BAND);
}

void TargetCodeGenerator::visit(BXorInstr& instr) {
  emitBinary(instr, Opcode::BXOR);
}

void TargetCodeGenerator::visit(SLenInstr& instr) {
  assert(!"TODO: SLenInstr CG");
}

void TargetCodeGenerator::visit(SIsEmptyInstr& instr) {
  assert(!"TODO: SIsEmptyInstr CG");
}

void TargetCodeGenerator::visit(SAddInstr& instr) {
  emitBinary(instr, Opcode::SADD);
}

void TargetCodeGenerator::visit(SSubStrInstr& instr) {
  assert(!"TODO: SSubStrInstr CG");
}

void TargetCodeGenerator::visit(SCmpEQInstr& instr) {
  emitBinary(instr, Opcode::SCMPEQ);
}

void TargetCodeGenerator::visit(SCmpNEInstr& instr) {
  emitBinary(instr, Opcode::SCMPNE);
}

void TargetCodeGenerator::visit(SCmpLEInstr& instr) {
  emitBinary(instr, Opcode::SCMPLE);
}

void TargetCodeGenerator::visit(SCmpGEInstr& instr) {
  emitBinary(instr, Opcode::SCMPGE);
}

void TargetCodeGenerator::visit(SCmpLTInstr& instr) {
  emitBinary(instr, Opcode::SCMPLT);
}

void TargetCodeGenerator::visit(SCmpGTInstr& instr) {
  emitBinary(instr, Opcode::SCMPGT);
}

void TargetCodeGenerator::visit(SCmpREInstr& instr) {
  assert(dynamic_cast<ConstantRegExp*>(instr.operand(1)) &&
         "RHS must be a ConstantRegExp");

  ConstantRegExp* re = static_cast<ConstantRegExp*>(instr.operand(1));

  Register a = allocate(1, instr);
  Register b = getRegister(instr.operand(0));
  Operand c = cp_.makeRegExp(re->get());

  emit(Opcode::SREGMATCH, a, b, c);
}

void TargetCodeGenerator::visit(SCmpBegInstr& instr) {
  emitBinary(instr, Opcode::SCMPBEG);
}

void TargetCodeGenerator::visit(SCmpEndInstr& instr) {
  emitBinary(instr, Opcode::SCMPEND);
}

void TargetCodeGenerator::visit(SInInstr& instr) {
  emitBinary(instr, Opcode::SCONTAINS);
}

void TargetCodeGenerator::visit(PCmpEQInstr& instr) {
  emitBinary(instr, Opcode::PCMPEQ);
}

void TargetCodeGenerator::visit(PCmpNEInstr& instr) {
  emitBinary(instr, Opcode::PCMPNE);
}

void TargetCodeGenerator::visit(PInCidrInstr& instr) {
  emitBinary(instr, Opcode::PINCIDR);
}
// }}}

}  // namespace flow
}  // namespace xzero
