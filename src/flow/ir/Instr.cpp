// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/ir/Instr.h>
#include <xzero/flow/ir/ConstantValue.h>
#include <xzero/flow/ir/ConstantArray.h>
#include <xzero/flow/ir/IRBuiltinHandler.h>
#include <xzero/flow/ir/IRBuiltinFunction.h>
#include <xzero/flow/ir/BasicBlock.h>
#include <utility>
#include <assert.h>
#include <inttypes.h>

namespace xzero {
namespace flow {

using namespace vm;

Instr::Instr(const Instr& v)
    : Value(v), parent_(nullptr), operands_(v.operands_) {
  for (Value* op : operands_) {
    if (op) {
      op->addUse(this);
    }
  }
}

Instr::Instr(FlowType ty, const std::vector<Value*>& ops,
             const std::string& name)
    : Value(ty, name), parent_(nullptr), operands_(ops) {
  for (Value* op : operands_) {
    if (op) {
      op->addUse(this);
    }
  }
}

Instr::~Instr() {
  for (Value* op : operands_) {
    if (!op) continue;

    op->removeUse(this);

    if (!parent()) continue;

    if (BasicBlock* oldBB = dynamic_cast<BasicBlock*>(op)) {
      parent()->unlinkSuccessor(oldBB);
    }
  }
}

void Instr::addOperand(Value* value) {
  operands_.push_back(value);

  value->addUse(this);

  if (BasicBlock* newBB = dynamic_cast<BasicBlock*>(value)) {
    parent()->linkSuccessor(newBB);
  }
}

Value* Instr::setOperand(size_t i, Value* value) {
  Value* old = operands_[i];
  operands_[i] = value;

  if (old) {
    old->removeUse(this);

    if (BasicBlock* oldBB = dynamic_cast<BasicBlock*>(old)) {
      parent()->unlinkSuccessor(oldBB);
    }
  }

  if (value) {
    value->addUse(this);

    if (BasicBlock* newBB = dynamic_cast<BasicBlock*>(value)) {
      parent()->linkSuccessor(newBB);
    }
  }

  return old;
}

size_t Instr::replaceOperand(Value* old, Value* replacement) {
  size_t count = 0;

  for (size_t i = 0, e = operands_.size(); i != e; ++i) {
    if (operands_[i] == old) {
      setOperand(i, replacement);
      ++count;
    }
  }

  return count;
}

void Instr::clearOperands() {
  for (size_t i = 0, e = operands_.size(); i != e; ++i) {
    setOperand(i, nullptr);
  }

  operands_.clear();
}

void Instr::dumpOne(const char* mnemonic) {
  if (type() != FlowType::Void) {
    printf("\t%%%s = %s", name().c_str() ?: "?", mnemonic);
  } else {
    printf("\t%s", mnemonic);
  }

  for (size_t i = 0, e = operands_.size(); i != e; ++i) {
    printf(i ? ", " : " ");
    Value* arg = operands_[i];
    if (dynamic_cast<Constant*>(arg)) {
      if (auto i = dynamic_cast<ConstantInt*>(arg)) {
        printf("%" PRIi64 "", i->get());
        continue;
      }
      if (auto s = dynamic_cast<ConstantString*>(arg)) {
        printf("\"%s\"", s->get().c_str());
        continue;
      }
      if (auto ip = dynamic_cast<ConstantIP*>(arg)) {
        printf("%s", ip->get().c_str());
        continue;
      }
      if (auto cidr = dynamic_cast<ConstantCidr*>(arg)) {
        printf("%s", cidr->get().str().c_str());
        continue;
      }
      if (auto re = dynamic_cast<ConstantRegExp*>(arg)) {
        printf("/%s/", re->get().pattern().c_str());
        continue;
      }
      if (auto bh = dynamic_cast<IRBuiltinHandler*>(arg)) {
        printf("%s", bh->get().to_s().c_str());
        continue;
      }
      if (auto bf = dynamic_cast<IRBuiltinFunction*>(arg)) {
        printf("%s", bf->get().to_s().c_str());
        continue;
      }
      if (auto ar = dynamic_cast<ConstantArray*>(arg)) {
        printf("[");
        size_t i = 0;
        switch (ar->type()) {
          case FlowType::IntArray:
            for (const auto& v : ar->get()) {
              if (i) printf(", ");
              printf("%" PRIi64 "", static_cast<ConstantInt*>(v)->get());
              ++i;
            }
            break;
          case FlowType::StringArray:
            for (const auto& v : ar->get()) {
              if (i) printf(", ");
              printf("\"%s\"", static_cast<ConstantString*>(v)->get().c_str());
              ++i;
            }
            break;
          case FlowType::IPAddrArray:
            for (const auto& v : ar->get()) {
              if (i) printf(", ");
              printf("%s", static_cast<ConstantIP*>(v)->get().str().c_str());
              ++i;
            }
            break;
          case FlowType::CidrArray:
            for (const auto& v : ar->get()) {
              if (i) printf(", ");
              printf("\"%s\"",
                     static_cast<ConstantCidr*>(v)->get().str().c_str());
              ++i;
            }
            break;
          default:
            abort();
        }
        printf("]");
        continue;
      }
    }
    printf("%%%s", arg->name().c_str());
  }
  printf("\n");
}

}  // namespace flow
}  // namespace xzero
