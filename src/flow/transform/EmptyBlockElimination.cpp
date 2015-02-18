// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/transform/EmptyBlockElimination.h>
#include <xzero/flow/ir/BasicBlock.h>
#include <xzero/flow/ir/IRHandler.h>
#include <xzero/flow/ir/Instructions.h>
#include <list>

namespace xzero {
namespace flow {

bool EmptyBlockElimination::run(IRHandler* handler) {
  std::list<BasicBlock*> eliminated;

  for (BasicBlock* bb : handler->basicBlocks()) {
    if (bb->instructions().size() != 1) continue;

    if (BrInstr* br = dynamic_cast<BrInstr*>(bb->getTerminator())) {
      BasicBlock* newSuccessor = br->targetBlock();
      eliminated.push_back(bb);
      if (bb == handler->getEntryBlock()) {
        handler->setEntryBlock(bb);
        break;
      } else {
        for (BasicBlock* pred : bb->predecessors()) {
          pred->getTerminator()->replaceOperand(bb, newSuccessor);
        }
      }
    }
  }

  for (BasicBlock* bb : eliminated) {
    bb->parent()->erase(bb);
  }

  return eliminated.size() > 0;
}

}  // namespace flow
}  // namespace xzero
