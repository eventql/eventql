// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/vm/Runtime.h>
#include <xzero/flow/vm/NativeCallback.h>
#include <xzero/flow/ir/IRProgram.h>
#include <xzero/flow/ir/IRHandler.h>
#include <xzero/flow/ir/BasicBlock.h>
#include <xzero/flow/ir/Instructions.h>

namespace xzero {
namespace flow {
namespace vm {

Runtime::~Runtime() {
  for (auto b : builtins_) {
    delete b;
  }
}

NativeCallback& Runtime::registerHandler(const std::string& name) {
  builtins_.push_back(new NativeCallback(this, name));
  return *builtins_[builtins_.size() - 1];
}

NativeCallback& Runtime::registerFunction(const std::string& name,
                                          FlowType returnType) {
  builtins_.push_back(new NativeCallback(this, name, returnType));
  return *builtins_[builtins_.size() - 1];
}

NativeCallback* Runtime::find(const std::string& signature) {
  for (auto callback : builtins_) {
    if (callback && callback->signature().to_s() == signature) {
      return callback;
    }
  }

  return nullptr;
}

NativeCallback* Runtime::find(const Signature& signature) {
  return find(signature.to_s());
}

void Runtime::unregisterNative(const std::string& name) {
  for (size_t i = 0, e = builtins_.size(); i != e; ++i) {
    if (builtins_[i] && builtins_[i]->signature().name() == name) {
      delete builtins_[i];
      builtins_[i] = nullptr;
      return;
    }
  }
}

bool Runtime::verify(IRProgram* program) {
  std::list<std::pair<Instr*, NativeCallback*>> calls;

  for (IRHandler* handler : program->handlers()) {
    for (BasicBlock* bb : handler->basicBlocks()) {
      for (Instr* instr : bb->instructions()) {
        if (auto ci = dynamic_cast<CallInstr*>(instr)) {
          if (auto native = find(ci->callee()->signature())) {
            calls.push_back(std::make_pair(instr, native));
          }
        } else if (auto hi = dynamic_cast<HandlerCallInstr*>(instr)) {
          if (auto native = find(hi->callee()->signature())) {
            calls.push_back(std::make_pair(instr, native));
          }
        }
      }
    }
  }

  for (auto call : calls) {
    if (!call.second->verify(call.first)) {
      return false;
    }
  }

  return true;
}

}  // namespace vm
}  // namespace flow
}  // namespace xzero
