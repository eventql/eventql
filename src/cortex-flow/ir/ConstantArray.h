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

namespace cortex {
namespace flow {

class CORTEX_FLOW_API ConstantArray : public Constant {
 public:
  ConstantArray(const std::vector<Constant*>& elements,
                const std::string& name = "")
      : Constant(makeArrayType(elements.front()->type()), name),
        elements_(elements) {}

  const std::vector<Constant*>& get() const { return elements_; }

  FlowType elementType() const { return elements_[0]->type(); }

 private:
  std::vector<Constant*> elements_;

  FlowType makeArrayType(FlowType elementType);
};

}  // namespace flow
}  // namespace cortex
