// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/Api.h>
#include <cortex-flow/ir/Constant.h>
#include <cortex-base/net/IPAddress.h>
#include <cortex-base/net/Cidr.h>
#include <cortex-base/RegExp.h>

#include <string>

namespace cortex {
namespace flow {

template <typename T, const FlowType Ty>
class CORTEX_FLOW_API ConstantValue : public Constant {
 public:
  ConstantValue(const T& value, const std::string& name = "")
      : Constant(Ty, name), value_(value) {}

  T get() const { return value_; }

 private:
  T value_;
};

typedef ConstantValue<int64_t, FlowType::Number> ConstantInt;
typedef ConstantValue<bool, FlowType::Boolean> ConstantBoolean;
typedef ConstantValue<std::string, FlowType::String> ConstantString;
typedef ConstantValue<IPAddress, FlowType::IPAddress> ConstantIP;
typedef ConstantValue<Cidr, FlowType::Cidr> ConstantCidr;
typedef ConstantValue<RegExp, FlowType::RegExp> ConstantRegExp;

}  // namespace flow
}  // namespace cortex
