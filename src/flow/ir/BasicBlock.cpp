// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/ir/BasicBlock.h>
#include <xzero/flow/ir/Instr.h>
#include <xzero/flow/ir/Instructions.h>
#include <xzero/flow/ir/IRHandler.h>
#include <algorithm>
#include <assert.h>
#include <math.h>

/*
 * TODO assert() on last instruction in current BB is not a terminator instr.
 */

namespace xzero {
namespace flow {

BasicBlock::BasicBlock(const std::string& name)
    : Value(FlowType::Void, name),
      parent_(nullptr),
      code_(),
      predecessors_(),
      successors_() {}

BasicBlock::~BasicBlock() {
  for (auto i = code_.rbegin(), e = code_.rend(); i != e; ++i) {
    delete *i;
  }
  code_.clear();

  assert(predecessors().empty() &&
         "Cannot remove a basic block that some other basic block still refers "
         "to.");
  for (BasicBlock* bb : predecessors()) {
    bb->unlinkSuccessor(this);
  }

  for (BasicBlock* bb : successors()) {
    unlinkSuccessor(bb);
  }
}

TerminateInstr* BasicBlock::getTerminator() const {
  return dynamic_cast<TerminateInstr*>(code_.back());
}

Instr* BasicBlock::remove(Instr* instr) {
  // if we're removing the terminator instruction
  if (instr == getTerminator()) {
    // then unlink all successors also
    for (Value* operand : instr->operands()) {
      if (BasicBlock* bb = dynamic_cast<BasicBlock*>(operand)) {
        unlinkSuccessor(bb);
      }
    }
  }

  auto i = std::find(code_.begin(), code_.end(), instr);
  assert(i != code_.end());
  code_.erase(i);
  instr->setParent(nullptr);

  return instr;
}

void BasicBlock::push_back(Instr* instr) {
  assert(instr != nullptr);
  assert(instr->parent() == nullptr);

  instr->setParent(this);
  code_.push_back(instr);

  // FIXME: do we need this? I'd say NOPE.
  setType(instr->type());

  // are we're now adding the terminate instruction?
  if (dynamic_cast<TerminateInstr*>(instr)) {
    // then check for possible successors
    for (auto operand : instr->operands()) {
      if (BasicBlock* bb = dynamic_cast<BasicBlock*>(operand)) {
        linkSuccessor(bb);
      }
    }
  }
}

void BasicBlock::merge_back(BasicBlock* bb) {
  assert(getTerminator() == nullptr);

  for (Instr* i : bb->code_) {
    push_back(i->clone());
  }
}

void BasicBlock::moveAfter(BasicBlock* otherBB) {
  assert(parent() == otherBB->parent());

  IRHandler* handler = parent();
  auto& list = handler->basicBlocks();

  list.remove(otherBB);

  auto i = std::find(list.begin(), list.end(), this);
  ++i;
  list.insert(i, otherBB);
}

void BasicBlock::moveBefore(BasicBlock* otherBB) {
  assert(parent() == otherBB->parent());

  IRHandler* handler = parent();
  auto& list = handler->basicBlocks();

  list.remove(otherBB);

  auto i = std::find(list.begin(), list.end(), this);
  list.insert(i, otherBB);
}

bool BasicBlock::isAfter(const BasicBlock* otherBB) const {
  assert(parent() == otherBB->parent());

  const auto& list = parent()->basicBlocks();
  auto i = std::find(list.cbegin(), list.cend(), this);

  if (i == list.cend()) return false;

  ++i;

  if (i == list.cend()) return false;

  return *i == otherBB;
}

void BasicBlock::dump() {
  int n = printf("%%%s:", name().c_str());
  if (!predecessors().empty()) {
    printf("%*c; [preds: ", 20 - n, ' ');
    for (size_t i = 0, e = predecessors().size(); i != e; ++i) {
      if (i) printf(", ");
      printf("%%%s", predecessors()[i]->name().c_str());
    }
    printf("]");
  }
  printf("\n");

  if (!successors().empty()) {
    printf("%20c; [succs: ", ' ');
    for (size_t i = 0, e = successors().size(); i != e; ++i) {
      if (i) printf(", ");
      printf("%%%s", successors()[i]->name().c_str());
    }
    printf("]\n");
  }

  for (size_t i = 0, e = code_.size(); i != e; ++i) {
    code_[i]->dump();
  }

  printf("\n");
}

void BasicBlock::linkSuccessor(BasicBlock* successor) {
  assert(successor != nullptr);

  successors().push_back(successor);
  successor->predecessors().push_back(this);
}

void BasicBlock::unlinkSuccessor(BasicBlock* successor) {
  assert(successor != nullptr);

  auto p = std::find(successor->predecessors_.begin(),
                     successor->predecessors_.end(), this);
  assert(p != successor->predecessors_.end());
  successor->predecessors_.erase(p);

  auto s = std::find(successors_.begin(), successors_.end(), successor);
  assert(s != successors_.end());
  successors_.erase(s);
}

std::vector<BasicBlock*> BasicBlock::dominators() {
  std::vector<BasicBlock*> result;
  collectIDom(result);
  result.push_back(this);
  return result;
}

std::vector<BasicBlock*> BasicBlock::immediateDominators() {
  std::vector<BasicBlock*> result;
  collectIDom(result);
  return result;
}

void BasicBlock::collectIDom(std::vector<BasicBlock*>& output) {
  for (BasicBlock* p : predecessors()) {
    p->collectIDom(output);
  }
}

void BasicBlock::verify() {
  return;

  if (code_.size() < 1) {
    fprintf(stderr,
            "BasicBlock.verify: Must contain at least one instruction.");
    abort();
  }

  for (size_t i = 0, e = code_.size() - 1; i != e; ++i) {
    if (dynamic_cast<TerminateInstr*>(code_[i]) != nullptr) {
      fprintf(stderr,
              "BasicBlock.verify: Found a terminate instruction in the middle "
              "of the block.");
      abort();
    }
  }

  if (getTerminator() == nullptr) {
    fprintf(stderr,
            "BasicBlock.verify: Last instruction must be a terminator "
            "instruction.");
    abort();
  }
}

}  // namespace flow
}  // namespace xzero
