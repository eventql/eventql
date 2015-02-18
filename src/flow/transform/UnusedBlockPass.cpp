// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/transform/UnusedBlockPass.h>
#include <xzero/flow/ir/BasicBlock.h>
#include <xzero/flow/ir/Instructions.h>
#include <xzero/flow/ir/IRHandler.h>
#include <xzero/flow/ir/Instructions.h>
#include <list>

namespace xzero {
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
}  // namespace xzero
