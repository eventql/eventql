// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/FlowType.h>
#include <sys/param.h>  // size_t, odd that it's not part of <stdint.h>.
#include <stdint.h>
#include <vector>

namespace cortex {
namespace flow {
namespace vm {

enum Opcode {
  // misc
  NOP = 0,  // NOP                 ; no operation

  // control
  EXIT,  // EXIT imm            ; exit program
  JMP,   // JMP imm             ; unconditional jump
  JN,    // JN reg, imm         ; conditional jump (A != 0)
  JZ,    // JZ reg, imm         ; conditional jump (A == 0)

  // copy
  MOV,  // A = B

  // const arrays
  ITCONST,  // A = intArray[B]
  STCONST,  // A = stringArray[B]
  PTCONST,  // A = ipaddrArray[B]
  CTCONST,  // A = cidrArray[B]

  // numerical
  IMOV,    // A = B/imm
  NCONST,  // A = numberConstants[B]
  NNEG,    // A = -A
  NNOT,    // A = ~A
  NADD,    // A = B + C
  NSUB,    // A = B - C
  NMUL,    // A = B * C
  NDIV,    // A = B / C
  NREM,    // A = B % C
  NSHL,    // A = B << C
  NSHR,    // A = B >> C
  NPOW,    // A = B ** C
  NAND,    // A = B & C
  NOR,     // A = B | C
  NXOR,    // A = B ^ C
  NCMPZ,   // A = B == 0
  NCMPEQ,  // A = B == C
  NCMPNE,  // A = B != C
  NCMPLE,  // A = B <= C
  NCMPGE,  // A = B >= C
  NCMPLT,  // A = B < C
  NCMPGT,  // A = B > C

  // numerical (reg, imm)
  NIADD,    // A = B + C
  NISUB,    // A = B - C
  NIMUL,    // A = B * C
  NIDIV,    // A = B / C
  NIREM,    // A = B % C
  NISHL,    // A = B << C
  NISHR,    // A = B >> C
  NIPOW,    // A = B ** C
  NIAND,    // A = B & C
  NIOR,     // A = B | C
  NIXOR,    // A = B ^ C
  NICMPEQ,  // A = B == C
  NICMPNE,  // A = B != C
  NICMPLE,  // A = B <= C
  NICMPGE,  // A = B >= C
  NICMPLT,  // A = B < C
  NICMPGT,  // A = B > C

  // boolean
  BNOT,  // A = !A
  BAND,  // A = B and C
  BOR,   // A = B or C
  BXOR,  // A = B xor C

  // string
  SCONST,     // A = stringConstants[B]
  SADD,       // A = B + C
  SADDMULTI,  // A = concat(B /*rbase*/, C /*count*/)
  SSUBSTR,    // A = substr(B, C /*offset*/, C+1 /*count*/)
  SCMPEQ,     // A = B == C
  SCMPNE,     // A = B != C
  SCMPLE,     // A = B <= C
  SCMPGE,     // A = B >= C
  SCMPLT,     // A = B < C
  SCMPGT,     // A = B > C
  SCMPBEG,    // A = B =^ C           /* B begins with C */
  SCMPEND,    // A = B =$ C           /* B ends with C */
  SCONTAINS,  // A = B in C           /* B is contained in C */
  SLEN,       // A = strlen(B)
  SISEMPTY,   // A = strlen(B) == 0
  SMATCHEQ,   // $pc = MatchSame[A].evaluate(B);
  SMATCHBEG,  // $pc = MatchBegin[A].evaluate(B);
  SMATCHEND,  // $pc = MatchEnd[A].evaluate(B);
  SMATCHR,    // $pc = MatchRegEx[A].evaluate(B);

  // IP address
  PCONST,   // A = ipconst[B]
  PCMPEQ,   // A = ip(B) == ip(C)
  PCMPNE,   // A = ip(B) != ip(C)
  PINCIDR,  // A = cidr(C).contains(ip(B))

  // CIDR
  CCONST,  // A = cidr(C)

  // regex
  SREGMATCH,  // A = B =~ C           /* regex match against regexPool[C] */
  SREGGROUP,  // A = regex.match(B)   /* regex match result */

  // conversion
  I2S,      // A = itoa(B)
  P2S,      // A = ip(B).toString();
  C2S,      // A = cidr(B).toString();
  R2S,      // A = regex(B).toString();
  S2I,      // A = atoi(B)
  SURLENC,  // A = urlencode(B)
  SURLDEC,  // A = urldecode(B)

  // invokation
  // CALL A = id, B = argc, C = rbase for argv
  CALL,     // [C+0] = functions[A] ([C+1 ... C+B])
  HANDLER,  // handlers[A] ([C+1 ... C+B]); if ([C+0] == true) EXIT 1
};

enum class InstructionSig {
  None = 0,  //                   ()
  R,         // reg               (A)
  RR,        // reg, reg          (AB)
  RRR,       // reg, reg, reg     (ABC)
  RI,        // reg, imm16        (AB)
  RRI,       // reg, reg, imm16   (ABC)
  RII,       // reg, imm16, imm16 (ABC)
  RIR,       // reg, imm16, reg   (ABC)
  IRR,       // imm16, reg, reg   (ABC)
  IIR,       // imm16, imm16, reg (ABC)
  I,         // imm16             (A)
};

typedef uint64_t Instruction;
typedef uint16_t Operand;
typedef uint16_t ImmOperand;

// --------------------------------------------------------------------------
// encoder

constexpr Instruction makeInstruction(Opcode opc) { return (Instruction)opc; }
constexpr Instruction makeInstruction(Opcode opc, Operand op1) {
  return (opc | (op1 << 16));
}
constexpr Instruction makeInstruction(Opcode opc, Operand op1, Operand op2) {
  return (opc | (op1 << 16) | (Instruction(op2) << 32));
}
constexpr Instruction makeInstruction(Opcode opc, Operand op1, Operand op2,
                                      Operand op3) {
  return (opc | (op1 << 16) | (Instruction(op2) << 32) |
          (Instruction(op3) << 48));
}

// --------------------------------------------------------------------------
// decoder

Buffer disassemble(Instruction pc, ImmOperand ip,
                   const char* comment = nullptr);
Buffer disassemble(const Instruction* program, size_t n);

constexpr Opcode opcode(Instruction instr) {
  return static_cast<Opcode>(instr & 0xFF);
}
constexpr Operand operandA(Instruction instr) {
  return static_cast<Operand>((instr >> 16) & 0xFFFF);
}
constexpr Operand operandB(Instruction instr) {
  return static_cast<Operand>((instr >> 32) & 0xFFFF);
}
constexpr Operand operandC(Instruction instr) {
  return static_cast<Operand>((instr >> 48) & 0xFFFF);
}

CORTEX_FLOW_API InstructionSig operandSignature(Opcode opc);
CORTEX_FLOW_API const char* mnemonic(Opcode opc);
CORTEX_FLOW_API size_t computeRegisterCount(const Instruction* code, size_t size);
CORTEX_FLOW_API size_t registerMax(Instruction instr);
CORTEX_FLOW_API FlowType resultType(Opcode opc);

}  // namespace vm
}  // namespace flow
}  // namespace cortex
