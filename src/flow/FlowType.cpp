// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/FlowType.h>

namespace xzero {
namespace flow {

std::string tos(FlowType type) {
  switch (type) {
    case FlowType::Void:
      return "void";
    case FlowType::Boolean:
      return "bool";
    case FlowType::Number:
      return "int";
    case FlowType::String:
      return "string";
    case FlowType::IPAddress:
      return "IPAddress";
    case FlowType::Cidr:
      return "Cidr";
    case FlowType::RegExp:
      return "RegExp";
    case FlowType::Handler:
      return "HandlerRef";
    case FlowType::IntArray:
      return "IntArray";
    case FlowType::StringArray:
      return "StringArray";
    case FlowType::IPAddrArray:
      return "IPAddrArray";
    case FlowType::CidrArray:
      return "CidrArray";
    default:
      return "";
  }
}

}  // namespace flow
}  // namespace xzero
