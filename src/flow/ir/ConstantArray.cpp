// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/ir/ConstantArray.h>
#include <cstdlib>

namespace xzero {
namespace flow {

FlowType ConstantArray::makeArrayType(FlowType elementType) {
  switch (elementType) {
    case FlowType::Number:
      return FlowType::IntArray;
    case FlowType::String:
      return FlowType::StringArray;
    case FlowType::IPAddress:
      return FlowType::IPAddrArray;
    case FlowType::Cidr:
      return FlowType::CidrArray;
    case FlowType::Boolean:
    case FlowType::RegExp:
    case FlowType::Handler:
    case FlowType::IntArray:
    case FlowType::StringArray:
    case FlowType::IPAddrArray:
    case FlowType::CidrArray:
    case FlowType::Void:
      abort();
    default:
      abort();
  }
}

}  // namespace flow
}  // namespace xzero
