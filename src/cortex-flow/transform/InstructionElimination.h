// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/Api.h>
#include <cortex-flow/ir/HandlerPass.h>

namespace cortex {
namespace flow {

class BasicBlock;

/**
 * Eliminates empty blocks, that are just jumping to the next block.
 */
class CORTEX_FLOW_API InstructionElimination : public HandlerPass {
 public:
  InstructionElimination() : HandlerPass("InstructionElimination") {}

  bool run(IRHandler* handler) override;

 private:
  bool rewriteCondBrToSameBranches(BasicBlock* bb);
  bool eliminateLinearBr(BasicBlock* bb);
  bool foldConstantCondBr(BasicBlock* bb);
  bool branchToExit(BasicBlock* bb);
};

}  // namespace flow
}  // namespace cortex
