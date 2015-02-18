// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/vm/Runner.h>
#include <xzero/flow/vm/Params.h>
#include <xzero/flow/vm/NativeCallback.h>
#include <xzero/flow/vm/Handler.h>
#include <xzero/flow/vm/Program.h>
#include <xzero/flow/vm/Match.h>
#include <xzero/flow/vm/Instruction.h>
#include <xzero/sysconfig.h>
#include <vector>
#include <utility>
#include <memory>
#include <new>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <inttypes.h>

#if 0 // !defined(NDEBUG)
#define TRACE(level, msg...) XZERO_DEBUG("vm", (level), msg)
#else
#define TRACE(msg...) do {} while (0)
#endif

namespace xzero {
namespace flow {
namespace vm {

std::unique_ptr<Runner> Runner::create(Handler* handler) {
  Runner* p = (Runner*)malloc(sizeof(Runner) +
                              handler->registerCount() * sizeof(uint64_t));
  new (p) Runner(handler);
  return std::unique_ptr<Runner>(p);
}

static FlowString* t = nullptr;

Runner::Runner(Handler* handler)
    : handler_(handler),
      program_(handler->program()),
      userdata_(nullptr),
      state_(Inactive),
      pc_(0),
      stringGarbage_() {
  // initialize emptyString()
  t = newString("");

  // initialize registers
  memset(data_, 0, sizeof(Register) * handler_->registerCount());
}

Runner::~Runner() {}

void Runner::operator delete(void* p) { free(p); }

FlowString* Runner::newString(const std::string& value) {
  stringGarbage_.push_back(Buffer(value.c_str(), value.size()));
  return &stringGarbage_.back();
}

FlowString* Runner::newString(const char* p, size_t n) {
  stringGarbage_.push_back(Buffer(p, n));
  return &stringGarbage_.back();
}

FlowString* Runner::catString(const FlowString& a, const FlowString& b) {
  Buffer s(a.size() + b.size() + 1);
  s.push_back(a);
  s.push_back(b);

  stringGarbage_.push_back(std::move(s));

  return &stringGarbage_.back();
}

void Runner::suspend() {
  assert(state_ == Running);
  TRACE(1, "Suspending handler %s.", handler_->name().c_str());

  state_ = Suspended;
}

bool Runner::resume() {
  assert(state_ == Suspended);
  TRACE(1, "Resuming handler %s.", handler_->name().c_str());

  return loop();
}

bool Runner::run() {
  assert(state_ == Inactive);
  TRACE(1, "Running handler %s.", handler_->name().c_str());

  return loop();
}

bool Runner::loop() {
  const Program* program = handler_->program();

  state_ = Running;

#define OP opcode((Instruction) * pc)
#define A operandA((Instruction) * pc)
#define B operandB((Instruction) * pc)
#define C operandC((Instruction) * pc)

#define toString(R) (*(FlowString*)data_[R])
#define toIPAddress(R) (*(IPAddress*)data_[R])
#define toCidr(R) (*(Cidr*)data_[R])
#define toRegExp(R) (*(RegExp*)data_[R])
#define toNumber(R) ((FlowNumber)data_[R])

#define toStringPtr(R) ((FlowString*)data_[R])
#define toCidrPtr(R) ((Cidr*)data_[R])

#if defined(ENABLE_FLOW_DIRECT_THREADED_VM)
  auto& code = handler_->directThreadedCode();

#define instr(name) \
  l_##name : ++pc;  \
  TRACE(2, "%s",    \
        disassemble((Instruction) * pc, (pc - code.data()) / 2).c_str());

#define get_pc() ((pc - code.data()) / 2)
#define set_pc(offset)               \
  do {                               \
    pc = code.data() + (offset) * 2; \
  } while (0)
#define jump_to(offset) \
  do {                  \
    set_pc(offset);     \
    jump;               \
  } while (0)
#define jump goto*(void*)*pc
#define next goto*(void*)*++pc
#else
  const auto& code = handler_->code();

#define instr(name) \
  l_##name : TRACE(2, "%s", disassemble(*pc, pc - code.data()).c_str());

#define get_pc() (pc - code.data())
#define set_pc(offset)           \
  do {                           \
    pc = code.data() + (offset); \
  } while (0)
#define jump_to(offset) \
  do {                  \
    set_pc(offset);     \
    jump;               \
  } while (0)
#define jump goto* ops[OP]
#define next goto* ops[opcode(*++pc)]
#endif

// {{{ jump table
#define label(opcode) &&l_##opcode
  static const void* const ops[] = {
      // misc
      label(NOP),

      // control
      label(EXIT),      label(JMP),       label(JN),        label(JZ),

      // copy
      label(MOV),

      // array
      label(ITCONST),   label(STCONST),   label(PTCONST),   label(CTCONST),

      // numerical
      label(IMOV),      label(NCONST),    label(NNEG),      label(NNOT),
      label(NADD),      label(NSUB),      label(NMUL),      label(NDIV),
      label(NREM),      label(NSHL),      label(NSHR),      label(NPOW),
      label(NAND),      label(NOR),       label(NXOR),      label(NCMPZ),
      label(NCMPEQ),    label(NCMPNE),    label(NCMPLE),    label(NCMPGE),
      label(NCMPLT),    label(NCMPGT),

      // numerical (reg, imm)
      label(NIADD),     label(NISUB),     label(NIMUL),     label(NIDIV),
      label(NIREM),     label(NISHL),     label(NISHR),     label(NIPOW),
      label(NIAND),     label(NIOR),      label(NIXOR),     label(NICMPEQ),
      label(NICMPNE),   label(NICMPLE),   label(NICMPGE),   label(NICMPLT),
      label(NICMPGT),

      // boolean op
      label(BNOT),      label(BAND),      label(BOR),       label(BXOR),

      // string op
      label(SCONST),    label(SADD),      label(SADDMULTI), label(SSUBSTR),
      label(SCMPEQ),    label(SCMPNE),    label(SCMPLE),    label(SCMPGE),
      label(SCMPLT),    label(SCMPGT),    label(SCMPBEG),   label(SCMPEND),
      label(SCONTAINS), label(SLEN),      label(SISEMPTY),  label(SMATCHEQ),
      label(SMATCHBEG), label(SMATCHEND), label(SMATCHR),

      // ipaddr
      label(PCONST),    label(PCMPEQ),    label(PCMPNE),    label(PINCIDR),

      // cidr
      label(CCONST),

      // regex
      label(SREGMATCH), label(SREGGROUP),

      // conversion
      label(I2S),       label(P2S),       label(C2S),       label(R2S),
      label(S2I),       label(SURLENC),   label(SURLDEC),

      // invokation
      label(CALL),      label(HANDLER), };
// }}}
// {{{ direct threaded code initialization
#if defined(ENABLE_FLOW_DIRECT_THREADED_VM)
  if (code.empty()) {
    const auto& source = handler_->code();
    code.resize(source.size() * 2);

    uint64_t* pc = code.data();
    for (size_t i = 0, e = source.size(); i != e; ++i) {
      Instruction instr = source[i];

      *pc++ = (uint64_t)ops[opcode(instr)];
      *pc++ = instr;
    }
  }
// const void** pc = code.data();
#endif
  // }}}

  const auto* pc = code.data();
  set_pc(pc_);

  jump;

  // {{{ misc
  instr(NOP) { next; }
  // }}}
  // {{{ control
  instr(EXIT) {
    state_ = Inactive;
    return A != 0;
  }

  instr(JMP) { jump_to(A); }

  instr(JN) {
    if (data_[A] != 0) {
      jump_to(B);
    } else {
      next;
    }
  }

  instr(JZ) {
    if (data_[A] == 0) {
      jump_to(B);
    } else {
      next;
    }
  }
  // }}}
  // {{{ copy
  instr(MOV) {
    data_[A] = data_[B];
    next;
  }
  // }}}
  // {{{ array
  instr(ITCONST) {
    data_[A] = reinterpret_cast<Register>(&program->constants().getIntArray(B));
    next;
  }
  instr(STCONST) {
    data_[A] =
        reinterpret_cast<Register>(&program->constants().getStringArray(B));
    next;
  }
  instr(PTCONST) {
    data_[A] =
        reinterpret_cast<Register>(&program->constants().getIPAddressArray(B));
    next;
  }
  instr(CTCONST) {
    data_[A] =
        reinterpret_cast<Register>(&program->constants().getCidrArray(B));
    next;
  }
  // }}}
  // {{{ numerical
  instr(IMOV) {
    data_[A] = B;
    next;
  }

  instr(NCONST) {
    data_[A] = program->constants().getInteger(B);
    next;
  }

  instr(NNEG) {
    data_[A] = (Register)(-toNumber(B));
    next;
  }

  instr(NNOT) {
    data_[A] = (Register)(~toNumber(B));
    next;
  }

  instr(NADD) {
    data_[A] = static_cast<Register>(toNumber(B) + toNumber(C));
    next;
  }

  instr(NSUB) {
    data_[A] = static_cast<Register>(toNumber(B) - toNumber(C));
    next;
  }

  instr(NMUL) {
    data_[A] = static_cast<Register>(toNumber(B) * toNumber(C));
    next;
  }

  instr(NDIV) {
    data_[A] = static_cast<Register>(toNumber(B) / toNumber(C));
    next;
  }

  instr(NREM) {
    data_[A] = static_cast<Register>(toNumber(B) % toNumber(C));
    next;
  }

  instr(NSHL) {
    data_[A] = static_cast<Register>(toNumber(B) << toNumber(C));
    next;
  }

  instr(NSHR) {
    data_[A] = static_cast<Register>(toNumber(B) >> toNumber(C));
    next;
  }

  instr(NPOW) {
    data_[A] = static_cast<Register>(powl(toNumber(B), toNumber(C)));
    next;
  }

  instr(NAND) {
    data_[A] = data_[B] & data_[C];
    next;
  }

  instr(NOR) {
    data_[A] = data_[B] | data_[C];
    next;
  }

  instr(NXOR) {
    data_[A] = data_[B] ^ data_[C];
    next;
  }

  instr(NCMPZ) {
    data_[A] = static_cast<Register>(toNumber(B) == 0);
    next;
  }

  instr(NCMPEQ) {
    data_[A] = static_cast<Register>(toNumber(B) == toNumber(C));
    next;
  }

  instr(NCMPNE) {
    data_[A] = static_cast<Register>(toNumber(B) != toNumber(C));
    next;
  }

  instr(NCMPLE) {
    data_[A] = static_cast<Register>(toNumber(B) <= toNumber(C));
    next;
  }

  instr(NCMPGE) {
    data_[A] = static_cast<Register>(toNumber(B) >= toNumber(C));
    next;
  }

  instr(NCMPLT) {
    data_[A] = static_cast<Register>(toNumber(B) < toNumber(C));
    next;
  }

  instr(NCMPGT) {
    data_[A] = static_cast<Register>(toNumber(B) > toNumber(C));
    next;
  }
  // }}}
  // {{{ numerical binary (reg, imm)
  instr(NIADD) {
    data_[A] = static_cast<Register>(toNumber(B) + C);
    next;
  }

  instr(NISUB) {
    data_[A] = static_cast<Register>(toNumber(B) - C);
    next;
  }

  instr(NIMUL) {
    data_[A] = static_cast<Register>(toNumber(B) * C);
    next;
  }

  instr(NIDIV) {
    data_[A] = static_cast<Register>(toNumber(B) / C);
    next;
  }

  instr(NIREM) {
    data_[A] = static_cast<Register>(toNumber(B) % C);
    next;
  }

  instr(NISHL) {
    data_[A] = static_cast<Register>(toNumber(B) << C);
    next;
  }

  instr(NISHR) {
    data_[A] = static_cast<Register>(toNumber(B) >> C);
    next;
  }

  instr(NIPOW) {
    data_[A] = static_cast<Register>(powl(toNumber(B), C));
    next;
  }

  instr(NIAND) {
    data_[A] = data_[B] & C;
    next;
  }

  instr(NIOR) {
    data_[A] = data_[B] | C;
    next;
  }

  instr(NIXOR) {
    data_[A] = data_[B] ^ C;
    next;
  }

  instr(NICMPEQ) {
    data_[A] = static_cast<Register>(toNumber(B) == C);
    next;
  }

  instr(NICMPNE) {
    data_[A] = static_cast<Register>(toNumber(B) != C);
    next;
  }

  instr(NICMPLE) {
    data_[A] = static_cast<Register>(toNumber(B) <= C);
    next;
  }

  instr(NICMPGE) {
    data_[A] = static_cast<Register>(toNumber(B) >= C);
    next;
  }

  instr(NICMPLT) {
    data_[A] = static_cast<Register>(toNumber(B) < C);
    next;
  }

  instr(NICMPGT) {
    data_[A] = static_cast<Register>(toNumber(B) > C);
    next;
  }
  // }}}
  // {{{ boolean
  instr(BNOT) {
    data_[A] = (Register)(!toNumber(B));
    next;
  }

  instr(BAND) {
    data_[A] = toNumber(B) && toNumber(C);
    next;
  }

  instr(BOR) {
    data_[A] = toNumber(B) || toNumber(C);
    next;
  }

  instr(BXOR) {
    data_[A] = toNumber(B) ^ toNumber(C);
    next;
  }
  // }}}
  // {{{ string
  instr(SCONST) {  // A = stringConstTable[B]
    data_[A] = reinterpret_cast<Register>(&program->constants().getString(B));
    next;
  }

  instr(SADD) {  // A = concat(B, C)
    data_[A] = (Register)catString(toString(B), toString(C));
    next;
  }

  instr(SSUBSTR) {  // A = substr(B, C /*offset*/, C+1 /*count*/)
    data_[A] = (Register)newString(toString(B).substr(data_[C], data_[C + 1]));
    next;
  }

  instr(SADDMULTI) {  // TODO: A = concat(B /*rbase*/, C /*count*/)
    next;
  }

  instr(SCMPEQ) {
    data_[A] = toString(B) == toString(C);
    next;
  }

  instr(SCMPNE) {
    data_[A] = toString(B) != toString(C);
    next;
  }

  instr(SCMPLE) {
    data_[A] = toString(B) <= toString(C);
    next;
  }

  instr(SCMPGE) {
    data_[A] = toString(B) >= toString(C);
    next;
  }

  instr(SCMPLT) {
    data_[A] = toString(B) < toString(C);
    next;
  }

  instr(SCMPGT) {
    data_[A] = toString(B) > toString(C);
    next;
  }

  instr(SCMPBEG) {
    const auto& b = toString(B);
    const auto& c = toString(C);
    data_[A] = b.begins(c);
    next;
  }

  instr(SCMPEND) {
    const auto& b = toString(B);
    const auto& c = toString(C);
    data_[A] = b.ends(c);
    next;
  }

  instr(SCONTAINS) {
    data_[A] = toString(B).find(toString(C)) != FlowString::npos;
    next;
  }

  instr(SLEN) {
    data_[A] = toString(B).size();
    next;
  }

  instr(SISEMPTY) {
    data_[A] = toString(B).empty();
    next;
  }

  instr(SMATCHEQ) {
    auto result = program_->match(B)->evaluate(toStringPtr(A), this);
    jump_to(result);
  }

  instr(SMATCHBEG) {
    auto result = program_->match(B)->evaluate(toStringPtr(A), this);
    jump_to(result);
  }

  instr(SMATCHEND) {
    auto result = program_->match(B)->evaluate(toStringPtr(A), this);
    jump_to(result);
  }

  instr(SMATCHR) {
    auto result = program_->match(B)->evaluate(toStringPtr(A), this);
    jump_to(result);
  }
  // }}}
  // {{{ ipaddr
  instr(PCONST) {
    data_[A] =
        reinterpret_cast<Register>(&program->constants().getIPAddress(B));
    next;
  }

  instr(PCMPEQ) {
    data_[A] = toIPAddress(B) == toIPAddress(C);
    next;
  }

  instr(PCMPNE) {
    data_[A] = toIPAddress(B) != toIPAddress(C);
    next;
  }

  instr(PINCIDR) {
    const IPAddress& ipaddr = toIPAddress(B);
    const Cidr& cidr = toCidr(C);
    data_[A] = cidr.contains(ipaddr);
    next;
  }
  // }}}
  // {{{ cidr
  instr(CCONST) {
    data_[A] = reinterpret_cast<Register>(&program->constants().getCidr(B));
    next;
  }
  // }}}
  // {{{ regex
  instr(SREGMATCH) {  // A = B =~ C
    RegExpContext* cx = (RegExpContext*)userdata();
    data_[A] = program_->constants().getRegExp(C).match(
        toString(B), cx ? cx->regexMatch() : nullptr);

    next;
  }

  instr(SREGGROUP) {  // A = regex.group(B)
    FlowNumber position = toNumber(B);
    RegExpContext* cx = (RegExpContext*)userdata();
    RegExp::Result* rr = cx->regexMatch();
    const auto& match = rr->at(position);

    data_[A] = (Register)newString(match.first, match.second);

    next;
  }
  // }}}
  // {{{ conversion
  instr(S2I) {  // A = atoi(B)
    data_[A] = toString(B).toInt();
    next;
  }

  instr(I2S) {  // A = itoa(B)
    char buf[64];
    if (snprintf(buf, sizeof(buf), "%" PRIi64 "", (int64_t)data_[B]) > 0) {
      data_[A] = (Register)newString(buf);
    } else {
      data_[A] = (Register)emptyString();
    }
    next;
  }

  instr(P2S) {  // A = ip(B).toString()
    const IPAddress& ipaddr = toIPAddress(B);
    data_[A] = (Register)newString(ipaddr.str());
    next;
  }

  instr(C2S) {  // A = cidr(B).toString()
    const Cidr& cidr = toCidr(B);
    data_[A] = (Register)newString(cidr.str());
    next;
  }

  instr(R2S) {  // A = regex(B).toString()
    const RegExp& re = toRegExp(B);
    data_[A] = (Register)newString(re.pattern());
    next;
  }

  instr(SURLENC) {  // A = urlencode(B)
    // TODO
    next;
  }

  instr(SURLDEC) {  // B = urldecode(B)
    // TODO
    next;
  }
  // }}}
  // {{{ invokation
  instr(CALL) {  // IIR
    size_t id = A;
    int argc = B;
    Register* argv = &data_[C];

    Params args(argc, argv, this);
    TRACE(2, "Calling function: %s",
          handler_->program()->nativeFunction(id)->signature().to_s().c_str());
    handler_->program()->nativeFunction(id)->invoke(args);

    if (state_ == Suspended) {
      pc_ = get_pc() + 1;
      return false;
    }

    next;
  }

  instr(HANDLER) {  // IIR
    size_t id = A;
    int argc = B;
    Value* argv = &data_[C];

    Params args(argc, argv, this);
    TRACE(2, "Calling handler: %s",
          handler_->program()->nativeHandler(id)->signature().to_s().c_str());
    handler_->program()->nativeHandler(id)->invoke(args);
    const bool handled = (bool)argv[0];

    if (state_ == Suspended) {
      pc_ = get_pc() + 1;
      return false;
    }

    if (handled) {
      state_ = Inactive;
      return true;
    }

    next;
  }
  // }}}
}

}  // namespace vm
}  // namespace flow
}  // namespace xzero
