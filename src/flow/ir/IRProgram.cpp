// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/ir/IRProgram.h>
#include <xzero/flow/ir/IRHandler.h>
#include <xzero/flow/ir/HandlerPass.h>
#include <xzero/flow/ir/ConstantArray.h>
#include <assert.h>

namespace xzero {
namespace flow {

using namespace vm;

IRProgram::IRProgram()
    : modules_(),
      numbers_(),
      strings_(),
      ipaddrs_(),
      cidrs_(),
      regexps_(),
      builtinFunctions_(),
      builtinHandlers_(),
      handlers_(),
      trueLiteral_(new ConstantBoolean(true, "trueLiteral")),
      falseLiteral_(new ConstantBoolean(false, "falseLiteral")) {}

IRProgram::~IRProgram() {
  for (auto& value : handlers_) delete value;
  for (auto& value : constantArrays_) delete value;
  for (auto& value : numbers_) delete value;
  for (auto& value : strings_) delete value;
  for (auto& value : ipaddrs_) delete value;
  for (auto& value : cidrs_) delete value;
  for (auto& value : regexps_) delete value;
  for (auto& value : builtinHandlers_) delete value;
  for (auto& value : builtinFunctions_) delete value;

  delete trueLiteral_;
  delete falseLiteral_;
}

void IRProgram::dump() {
  printf("; IRProgram\n");

  for (auto handler : handlers_) handler->dump();
}

template <typename T, typename U>
T* IRProgram::get(std::vector<T*>& table, const U& literal) {
  for (size_t i = 0, e = table.size(); i != e; ++i)
    if (table[i]->get() == literal) return table[i];

  T* value = new T(literal);
  table.push_back(value);
  return value;
}

template FLOW_API ConstantInt* IRProgram::get<ConstantInt, int64_t>(
    std::vector<ConstantInt*>&, const int64_t&);
template FLOW_API ConstantArray* IRProgram::get<
    ConstantArray, std::vector<Constant*>>(std::vector<ConstantArray*>&,
                                           const std::vector<Constant*>&);
template FLOW_API ConstantString* IRProgram::get<ConstantString, std::string>(
    std::vector<ConstantString*>&, const std::string&);
template FLOW_API ConstantIP* IRProgram::get<ConstantIP, IPAddress>(
    std::vector<ConstantIP*>&, const IPAddress&);
template FLOW_API ConstantCidr* IRProgram::get<ConstantCidr, Cidr>(
    std::vector<ConstantCidr*>&, const Cidr&);
template FLOW_API ConstantRegExp* IRProgram::get<ConstantRegExp, RegExp>(
    std::vector<ConstantRegExp*>&, const RegExp&);
template FLOW_API IRBuiltinHandler* IRProgram::get<IRBuiltinHandler, Signature>(
    std::vector<IRBuiltinHandler*>&, const Signature&);
template FLOW_API IRBuiltinFunction* IRProgram::get<IRBuiltinFunction, Signature>(
    std::vector<IRBuiltinFunction*>&, const Signature&);

}  // namespace flow
}  // namespace xzero
