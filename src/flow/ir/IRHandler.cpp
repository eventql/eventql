// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/ir/IRHandler.h>
#include <xzero/flow/ir/BasicBlock.h>
#include <xzero/flow/ir/Instructions.h>
#include <algorithm>
#include <assert.h>

namespace xzero {
namespace flow {

using namespace vm;

IRHandler::IRHandler(const std::string& name)
    : Constant(FlowType::Handler, name), parent_(nullptr), blocks_() {}

IRHandler::~IRHandler() {
  for (BasicBlock* bb : blocks_) {
    for (Instr* instr : bb->instructions()) {
      instr->clearOperands();
    }
  }

  while (!blocks_.empty()) {
    auto i = blocks_.begin();
    auto e = blocks_.end();

    while (i != e) {
      BasicBlock* bb = *i;

      if (bb->predecessors().empty()) {
        delete bb;
        auto k = i;
        ++i;
        blocks_.erase(k);
      } else {
        // skip BBs that other BBs still point to (we never point to ourself).
        ++i;
      }
    }
  }
}

void IRHandler::setEntryBlock(BasicBlock* bb) {
  assert(bb->parent() == nullptr || bb->parent() == this);

  if (bb->parent() == this) {
    auto i = std::find(blocks_.begin(), blocks_.end(), bb);
    assert(i != blocks_.end());
    blocks_.erase(i);
  }

  blocks_.push_front(bb);
}

void IRHandler::dump() {
  printf(".handler %s %*c; entryPoint = %%%s\n", name().c_str(),
         10 - (int)name().size(), ' ', getEntryBlock()->name().c_str());

  for (BasicBlock* bb : blocks_) bb->dump();

  printf("\n");
}

void IRHandler::erase(BasicBlock* bb) {
  auto i = std::find(blocks_.begin(), blocks_.end(), bb);
  assert(i != blocks_.end() &&
         "Given basic block must be a member of this handler to be removed.");
  blocks_.erase(i);

  if (TerminateInstr* terminator = bb->getTerminator()) {
    delete bb->remove(terminator);
  }

  delete bb;
}

void IRHandler::verify() {
  for (BasicBlock* bb : blocks_) {
    bb->verify();
  }
}

}  // namespace flow
}  // namespace xzero
