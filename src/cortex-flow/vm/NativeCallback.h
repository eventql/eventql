// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/Api.h>
#include <cortex-flow/FlowType.h>
#include <cortex-flow/vm/Signature.h>
#include <cortex-base/net/IPAddress.h>
#include <cortex-base/net/Cidr.h>
#include <cortex-base/RegExp.h>
#include <string>
#include <vector>
#include <functional>

namespace cortex {
namespace flow {

class Instr;

namespace vm {

typedef uint64_t Value;

class Params;
class Runner;
class Runtime;

class CORTEX_FLOW_API NativeCallback {
 public:
  typedef std::function<void(Params& args)> Functor;
  typedef std::function<bool(Instr*)> Verifier;

 private:
  Runtime* runtime_;
  bool isHandler_;
  Verifier verifier_;
  Functor function_;
  Signature signature_;

  // following attribs are irrelevant to the VM but useful for the frontend
  std::vector<std::string> names_;
  std::vector<void*> defaults_;

 public:
  NativeCallback(Runtime* runtime, const std::string& _name);
  NativeCallback(Runtime* runtime, const std::string& _name,
                 FlowType _returnType);
  ~NativeCallback();

  bool isHandler() const;
  const std::string name() const;
  const Signature& signature() const;

  // signature builder

  /** Declare the return type. */
  NativeCallback& returnType(FlowType type);

  /** Declare a single named parameter without default value. */
  template <typename T>
  NativeCallback& param(const std::string& name);

  /** Declare a single named parameter with default value. */
  template <typename T>
  NativeCallback& param(const std::string& name, T defaultValue);

  /** Declare ordered parameter signature. */
  template <typename... Args>
  NativeCallback& params(Args... args);

  // semantic verifier
  NativeCallback& verifier(const Verifier& vf);
  template <typename Class>
  NativeCallback& verifier(bool (Class::*method)(Instr*), Class* obj);
  template <typename Class>
  NativeCallback& verifier(bool (Class::*method)(Instr*));
  bool verify(Instr* call);

  // bind callback
  NativeCallback& bind(const Functor& cb);
  template <typename Class>
  NativeCallback& bind(void (Class::*method)(Params&), Class* obj);
  template <typename Class>
  NativeCallback& bind(void (Class::*method)(Params&));

  // named parameter handling
  bool isNamed() const;
  const std::string& getNameAt(size_t i) const;
  const void* getDefaultAt(size_t i) const;
  int find(const std::string& name) const;

  // runtime
  void invoke(Params& args) const;
};

// {{{ inlines
inline NativeCallback& NativeCallback::returnType(FlowType type) {
  signature_.setReturnType(type);
  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<bool>(const std::string& name) {
  signature_.args().push_back(FlowType::Boolean);
  names_.push_back(name);
  defaults_.push_back(nullptr /*no default value*/);

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<bool>(const std::string& name,
                                                   bool defaultValue) {
  signature_.args().push_back(FlowType::Boolean);
  names_.push_back(name);

  bool* value = new bool;
  *value = defaultValue;

  defaults_.push_back((void*)value);

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<FlowNumber>(
    const std::string& name) {
  signature_.args().push_back(FlowType::Number);
  names_.push_back(name);

  defaults_.push_back(nullptr /*no default value*/);

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<FlowNumber>(
    const std::string& name, FlowNumber defaultValue) {
  signature_.args().push_back(FlowType::Number);
  names_.push_back(name);

  FlowNumber* value = new FlowNumber;
  *value = defaultValue;
  defaults_.push_back(value);

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<int>(const std::string& name) {
  signature_.args().push_back(FlowType::Number);
  names_.push_back(name);
  defaults_.push_back(nullptr /*no default value*/);

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<int>(const std::string& name,
                                                  int defaultValue) {
  signature_.args().push_back(FlowType::Number);
  names_.push_back(name);

  FlowNumber* value = new FlowNumber;
  *value = defaultValue;
  defaults_.push_back((void*)value);

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<FlowString>(
    const std::string& name) {
  signature_.args().push_back(FlowType::String);
  names_.push_back(name);
  defaults_.push_back(nullptr /*no default value*/);

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<FlowString>(
    const std::string& name, FlowString defaultValue) {
  signature_.args().push_back(FlowType::String);
  names_.push_back(name);
  defaults_.push_back((void*)new FlowString(defaultValue));

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<IPAddress>(
    const std::string& name) {
  signature_.args().push_back(FlowType::IPAddress);
  names_.push_back(name);
  defaults_.push_back(nullptr /*no default value*/);

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<IPAddress>(
    const std::string& name, IPAddress defaultValue) {
  signature_.args().push_back(FlowType::IPAddress);
  names_.push_back(name);
  defaults_.push_back((void*)new IPAddress(defaultValue));

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<Cidr>(const std::string& name) {
  signature_.args().push_back(FlowType::Cidr);
  names_.push_back(name);
  defaults_.push_back(nullptr /*no default value*/);

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<Cidr>(const std::string& name,
                                                   Cidr defaultValue) {
  signature_.args().push_back(FlowType::Cidr);
  names_.push_back(name);
  defaults_.push_back((void*)new Cidr(defaultValue));

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<RegExp>(const std::string& name) {
  signature_.args().push_back(FlowType::RegExp);
  names_.push_back(name);
  defaults_.push_back(nullptr /*no default value*/);

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<RegExp>(const std::string& name,
                                                     RegExp defaultValue) {
  signature_.args().push_back(FlowType::RegExp);
  names_.push_back(name);
  defaults_.push_back((void*)new RegExp(defaultValue));

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<FlowIntArray>(
    const std::string& name) {
  assert(defaults_.size() == names_.size());

  signature_.args().push_back(FlowType::IntArray);
  names_.push_back(name);
  defaults_.push_back(nullptr /*no default value*/);

  return *this;
}

template <>
inline NativeCallback& NativeCallback::param<FlowStringArray>(
    const std::string& name) {
  assert(defaults_.size() == names_.size());

  signature_.args().push_back(FlowType::StringArray);
  names_.push_back(name);
  defaults_.push_back(nullptr /*no default value*/);

  return *this;
}

// ------------------------------------------------------------------------------------------

template <typename... Args>
inline NativeCallback& NativeCallback::params(Args... args) {
  signature_.setArgs({args...});
  return *this;
}

inline NativeCallback& NativeCallback::verifier(const Verifier& vf) {
  verifier_ = vf;
  return *this;
}

template <typename Class>
inline NativeCallback& NativeCallback::verifier(bool (Class::*method)(Instr*),
                                                Class* obj) {
  verifier_ = std::bind(method, obj, std::placeholders::_1);
  return *this;
}

template <typename Class>
inline NativeCallback& NativeCallback::verifier(bool (Class::*method)(Instr*)) {
  verifier_ =
      std::bind(method, static_cast<Class*>(runtime_), std::placeholders::_1);
  return *this;
}

inline bool NativeCallback::verify(Instr* call) {
  if (!verifier_) return true;

  return verifier_(call);
}

inline NativeCallback& NativeCallback::bind(const Functor& cb) {
  function_ = cb;
  return *this;
}

template <typename Class>
inline NativeCallback& NativeCallback::bind(void (Class::*method)(Params&),
                                            Class* obj) {
  function_ = std::bind(method, obj, std::placeholders::_1);
  return *this;
}

template <typename Class>
inline NativeCallback& NativeCallback::bind(void (Class::*method)(Params&)) {
  function_ =
      std::bind(method, static_cast<Class*>(runtime_), std::placeholders::_1);
  return *this;
}

inline bool NativeCallback::isNamed() const { return !names_.empty(); }

inline const std::string& NativeCallback::getNameAt(size_t i) const {
  assert(i < names_.size());
  return names_[i];
}

inline const void* NativeCallback::getDefaultAt(size_t i) const {
  return i < defaults_.size() ? defaults_[i] : nullptr;
}
// }}}

}  // namespace vm
}  // namespace flow
}  // namespace cortex
