// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/vm/NativeCallback.h>
#include <xzero/net/IPAddress.h>
#include <xzero/net/Cidr.h>
#include <xzero/RegExp.h>

namespace xzero {
namespace flow {
namespace vm {

// constructs a handler callback
NativeCallback::NativeCallback(Runtime* runtime, const std::string& _name)
    : runtime_(runtime),
      isHandler_(true),
      verifier_(),
      function_(),
      signature_() {
  signature_.setName(_name);
  signature_.setReturnType(FlowType::Boolean);
}

// constructs a function callback
NativeCallback::NativeCallback(Runtime* runtime, const std::string& _name,
                               FlowType _returnType)
    : runtime_(runtime),
      isHandler_(false),
      verifier_(),
      function_(),
      signature_() {
  signature_.setName(_name);
  signature_.setReturnType(_returnType);
}

NativeCallback::~NativeCallback() {
  for (size_t i = 0, e = defaults_.size(); i != e; ++i) {
    FlowType type = signature_.args()[i];
    switch (type) {
      case FlowType::Boolean:
        delete (bool*)defaults_[i];
        break;
      case FlowType::Number:
        delete (FlowNumber*)defaults_[i];
        break;
      case FlowType::String:
        delete (FlowString*)defaults_[i];
        break;
      case FlowType::IPAddress:
        delete (IPAddress*)defaults_[i];
        break;
      case FlowType::Cidr:
        delete (Cidr*)defaults_[i];
        break;
      case FlowType::RegExp:
        delete (RegExp*)defaults_[i];
        break;
      case FlowType::Handler:
      case FlowType::StringArray:
      case FlowType::IntArray:
      default:
        break;
    }
  }
}

bool NativeCallback::isHandler() const { return isHandler_; }

const std::string NativeCallback::name() const { return signature_.name(); }

const Signature& NativeCallback::signature() const { return signature_; }

int NativeCallback::find(const std::string& name) const {
  for (int i = 0, e = names_.size(); i != e; ++i)
    if (names_[i] == name) return i;

  return -1;
}

void NativeCallback::invoke(Params& args) const { function_(args); }

}  // namespace vm
}  // namespace flow
}  // namespace xzero
