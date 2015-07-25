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
#include <cortex-flow/vm/Program.h>
#include <cortex-flow/vm/Runner.h>
#include <cortex-base/net/IPAddress.h>
#include <cortex-base/net/Cidr.h>

namespace cortex {
namespace flow {
namespace vm {

class CORTEX_FLOW_API Params {
 private:
  int argc_;
  Register* argv_;
  Runner* caller_;

 public:
  Params(int argc, Register* argv, Runner* caller)
      : argc_(argc), argv_(argv), caller_(caller) {}

  Runner* caller() const { return caller_; }

  void setResult(bool value) { argv_[0] = value; }
  void setResult(Register value) { argv_[0] = value; }
  void setResult(FlowNumber value) { argv_[0] = (Register)value; }
  void setResult(Handler* handler) {
    argv_[0] = caller_->program()->indexOf(handler);
  }
  void setResult(const char* cstr) {
    argv_[0] = (Register)caller_->newString(cstr);
  }
  void setResult(const std::string& str) {
    argv_[0] = (Register)caller_->newString(str.data(), str.size());
  }
  void setResult(const FlowString& str) { argv_[0] = (Register) & str; }
  void setResult(const FlowString* str) { argv_[0] = (Register)str; }
  void setResult(const IPAddress* ip) { argv_[0] = (Register)ip; }
  void setResult(const Cidr* cidr) { argv_[0] = (Register)cidr; }

  int size() const { return argc_; }
  int count() const { return argc_; }

  Register at(size_t i) const { return argv_[i]; }
  Register operator[](size_t i) const { return argv_[i]; }
  Register& operator[](size_t i) { return argv_[i]; }

  bool getBool(size_t offset) const { return at(offset); }
  FlowNumber getInt(size_t offset) const { return at(offset); }
  const FlowString& getString(size_t offset) const {
    return *(FlowString*)at(offset);
  }
  Handler* getHandler(size_t offset) const {
    return caller_->program()->handler(at(offset));
  }
  const IPAddress& getIPAddress(size_t offset) const {
    return *(IPAddress*)at(offset);
  }
  const Cidr& getCidr(size_t offset) const { return *(Cidr*)at(offset); }

  const FlowIntArray& getIntArray(size_t offset) const {
    return *(FlowIntArray*)at(offset);
  }
  const FlowStringArray& getStringArray(size_t offset) const {
    return *(FlowStringArray*)at(offset);
  }
  const FlowIPAddrArray& getIPAddressArray(size_t offset) const {
    return *(FlowIPAddrArray*)at(offset);
  }
  const FlowCidrArray& getCidrArray(size_t offset) const {
    return *(FlowCidrArray*)at(offset);
  }

  class iterator {  // {{{
   private:
    Params* params_;
    size_t current_;

   public:
    iterator(Params* p, size_t init) : params_(p), current_(init) {}
    iterator(const iterator& v) = default;

    size_t offset() const { return current_; }
    Register get() const { return params_->at(current_); }

    Register& operator*() { return params_->argv_[current_]; }
    const Register& operator*() const { return params_->argv_[current_]; }

    iterator& operator++() {
      if (current_ != static_cast<decltype(current_)>(params_->argc_)) {
        ++current_;
      }
      return *this;
    }

    bool operator==(const iterator& other) const {
      return current_ == other.current_;
    }

    bool operator!=(const iterator& other) const {
      return current_ != other.current_;
    }
  };  // }}}

  iterator begin() { return iterator(this, std::min(1, argc_)); }
  iterator end() { return iterator(this, argc_); }
};

}  // namespace vm
}  // namespace flow
}  // namespace cortex
