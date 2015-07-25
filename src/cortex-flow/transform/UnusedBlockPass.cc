// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-flow/transform/UnusedBlockPass.h>
#include <cortex-flow/ir/BasicBlock.h>
#include <cortex-flow/ir/Instructions.h>
#include <cortex-flow/ir/IRHandler.h>
#include <cortex-flow/ir/Instructions.h>
#include <list>

namespace cortex {
namespace flow {

bool UnusedBlockPass::run(IRHandler* handler) {
  std::list<BasicBlock*> unused;

  for (BasicBlock* bb : handler->basicBlocks()) {
    if (bb == handler->getEntryBlock()) continue;

    if (!bb->predecessors().empty()) continue;

    unused.push_back(bb);
  }

  for (BasicBlock* bb : unused) {
    handler->erase(bb);
  }

  return !unused.empty();
}

}  // namespace flow
}  // namespace cortex
