// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/transform/InstructionElimination.h>
#include <xzero/flow/ir/BasicBlock.h>
#include <xzero/flow/ir/Instructions.h>
#include <xzero/flow/ir/IRHandler.h>
#include <xzero/flow/ir/Instructions.h>
#include <list>

namespace xzero {
namespace flow {

bool InstructionElimination::run(IRHandler* handler) {
  for (BasicBlock* bb : handler->basicBlocks()) {
    if (rewriteCondBrToSameBranches(bb)) return true;
    if (eliminateLinearBr(bb)) return true;
    if (foldConstantCondBr(bb)) return true;
    if (branchToExit(bb)) return true;
  }

  return false;
}

/*
 * Rewrites CONDBR (%foo, %foo) to BR (%foo) as both target branch pointers
 * point to the same branch.
 */
bool InstructionElimination::rewriteCondBrToSameBranches(BasicBlock* bb) {
  // attempt to eliminate useless condbr
  if (CondBrInstr* condbr = dynamic_cast<CondBrInstr*>(bb->getTerminator())) {
    if (condbr->trueBlock() != condbr->falseBlock()) return false;

    // printf("rewriteCondBrToSameBranches\n");
    // bb->dump();

    BasicBlock* nextBB = condbr->trueBlock();

    // remove old terminator
    delete bb->remove(condbr);

    // create new terminator
    bb->push_back(new BrInstr(nextBB));

    return true;
  }

  return false;
}

/*
 * Eliminates BR instructions to basic blocks that are only referenced by one
 * basic block
 * by eliminating the BR and merging the BR instructions target block at the end
 * of the current block.
 */
bool InstructionElimination::eliminateLinearBr(BasicBlock* bb) {
  // attempt to eliminate useless linear br
  if (BrInstr* br = dynamic_cast<BrInstr*>(bb->getTerminator())) {
    if (br->targetBlock()->predecessors().size() != 1) return false;

    if (br->targetBlock()->predecessors().front() != bb) return false;

    // we are the only predecessor of BR's target block, so merge them

    BasicBlock* nextBB = br->targetBlock();

    // remove old terminator
    delete bb->remove(br);

    // merge nextBB
    bb->merge_back(nextBB);

    // destroy unused BB
    bb->parent()->erase(nextBB);

    return true;
  }

  return false;
}

bool InstructionElimination::foldConstantCondBr(BasicBlock* bb) {
  if (auto condbr = dynamic_cast<CondBrInstr*>(bb->getTerminator())) {
    if (auto cond = dynamic_cast<ConstantBoolean*>(condbr->condition())) {
      std::pair<BasicBlock*, BasicBlock*> use;

      if (cond->get()) {
        // printf("condition is always true\n");
        use = std::make_pair(condbr->trueBlock(), condbr->falseBlock());
      } else {
        // printf("condition is always false\n");
        use = std::make_pair(condbr->falseBlock(), condbr->trueBlock());
      }

      bb->remove(condbr);
      bb->push_back(new BrInstr(use.first));
      delete condbr;
      return true;
    }
  }
  return false;
}

/*
 * Eliminates a superfluous BR instruction to a basic block that just exits.
 *
 * This will highly increase the number of exit points but reduce
 * the number of executed instructions for each path.
 */
bool InstructionElimination::branchToExit(BasicBlock* bb) {
  if (BrInstr* br = dynamic_cast<BrInstr*>(bb->getTerminator())) {
    BasicBlock* targetBB = br->targetBlock();

    if (targetBB->instructions().size() != 1) return false;

    if (bb->isAfter(targetBB)) return false;

    if (RetInstr* ret = dynamic_cast<RetInstr*>(targetBB->getTerminator())) {
      delete bb->remove(br);
      bb->push_back(ret->clone());

      return true;
    }
  }

  return false;
}

}  // namespace flow
}  // namespace xzero
