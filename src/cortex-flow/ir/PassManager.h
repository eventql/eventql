// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/Api.h>
#include <list>
#include <memory>

namespace cortex {
namespace flow {

class HandlerPass;
class IRHandler;
class IRProgram;

class CORTEX_FLOW_API PassManager {
 public:
  PassManager();
  ~PassManager();

  /** registers given pass to the pass manager.
   */
  void registerPass(std::unique_ptr<HandlerPass>&& handlerPass);

  /** runs passes on a complete program.
   */
  void run(IRProgram* program);

  /** runs passes on given handler.
   */
  void run(IRHandler* handler);

 private:
  std::list<std::unique_ptr<HandlerPass>> handlerPasses_;
};

}  // namespace flow
}  // namespace cortex
