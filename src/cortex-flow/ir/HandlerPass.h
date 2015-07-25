// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/Api.h>

namespace cortex {
namespace flow {

class IRHandler;

class CORTEX_FLOW_API HandlerPass {
 public:
  explicit HandlerPass(const char* name) : name_(name) {}

  virtual ~HandlerPass() {}

  /**
   * Runs this pass on given \p handler.
   *
   * @retval true The handler was modified.
   * @retval false The handler was not modified.
   */
  virtual bool run(IRHandler* handler) = 0;

  const char* name() const { return name_; }

 private:
  const char* name_;
};

}  // namespace flow
}  // namespace cortex
