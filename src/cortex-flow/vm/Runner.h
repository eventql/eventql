// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/Api.h>
#include <cortex-flow/FlowType.h>
#include <cortex-flow/vm/Handler.h>
#include <cortex-flow/vm/Instruction.h>
#include <cortex-base/CustomDataMgr.h>
#include <cortex-base/RegExp.h>
#include <utility>
#include <list>
#include <memory>
#include <new>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <string>

namespace cortex {
namespace flow {
namespace vm {

// ExecutionEngine
// VM
class CORTEX_FLOW_API Runner : public CustomData {
 public:
  enum State {
    Inactive,   //!< No handler running nor suspended.
    Running,    //!< Active handler is currently running.
    Suspended,  //!< Active handler is currently suspended.
  };

 private:
  Handler* handler_;
  Program* program_;

  //! pointer to the currently evaluated HttpRequest/HttpResponse our case
  std::pair<void*,void*> userdata_;

  RegExpContext regexpContext_;

  State state_;     //!< current VM state
  size_t pc_;       //!< last saved program execution offset

  std::list<Buffer> stringGarbage_;

  Register data_[];

 public:
  ~Runner();

  static std::unique_ptr<Runner> create(Handler* handler);
  static void operator delete(void* p);

  bool run();
  void suspend();
  bool resume();

  State state() const { return state_; }
  bool isInactive() const { return state_ == Inactive; }
  bool isRunning() const { return state_ == Running; }
  bool isSuspended() const { return state_ == Suspended; }

  Handler* handler() const { return handler_; }
  Program* program() const { return program_; }
  void* userdata() const { return userdata_.first; }
  void* userdata2() const { return userdata_.second; }
  void setUserData(void* p, void* q = nullptr) {
    userdata_.first = p;
    userdata_.second = q;
  }

  template<typename P, typename Q>
  inline void setUserData(std::pair<P, Q> udata) {
    setUserData(udata.first, udata.second);
  }

  const RegExpContext* regexpContext() const noexcept { return &regexpContext_; }
  RegExpContext* regexpContext() noexcept { return &regexpContext_; }

  const Register* data() const { return data_; }

  FlowString* newString(const std::string& value);
  FlowString* newString(const char* p, size_t n);
  FlowString* catString(const FlowString& a, const FlowString& b);
  const FlowString* emptyString() const { return &*stringGarbage_.begin(); }

 private:
  explicit Runner(Handler* handler);

  inline bool loop();

  Runner(Runner&) = delete;
  Runner& operator=(Runner&) = delete;
};

}  // namespace vm
}  // namespace flow
}  // namespace cortex
