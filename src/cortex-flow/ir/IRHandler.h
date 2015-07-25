// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/Api.h>
#include <cortex-flow/ir/Constant.h>

#include <string>
#include <vector>
#include <list>

namespace cortex {
namespace flow {

class BasicBlock;
class IRProgram;
class IRBuilder;

class CORTEX_FLOW_API IRHandler : public Constant {
 public:
  explicit IRHandler(const std::string& name);
  ~IRHandler();

  BasicBlock* createBlock(const std::string& name = "");

  IRProgram* parent() const { return parent_; }
  void setParent(IRProgram* prog) { parent_ = prog; }

  void dump() override;

  std::list<BasicBlock*>& basicBlocks() { return blocks_; }

  BasicBlock* getEntryBlock() const { return blocks_.front(); }
  void setEntryBlock(BasicBlock* bb);

  /**
   * Unlinks and deletes given basic block \p bb from handler.
   *
   * @note \p bb will be a dangling pointer after this call.
   */
  void erase(BasicBlock* bb);

  /**
   * Performs given transformation on this handler.
   *
   * @see HandlerPass
   */
  template <typename TheHandlerPass, typename... Args>
  size_t transform(Args&&... args) {
    return TheHandlerPass(args...).run(this);
  }

  /**
   * Performs sanity checks on internal data structures.
   *
   * This call does not return any success or failure as every failure is
   *considered fatal
   * and will cause the program to exit with diagnostics as this is most likely
   *caused by
   * an application programming error.
   *
   * @note Always call this on completely defined handlers and never on
   *half-contructed ones.
   */
  void verify();

 private:
  IRProgram* parent_;
  std::list<BasicBlock*> blocks_;

  friend class IRBuilder;
};

}  // namespace flow
}  // namespace cortex
