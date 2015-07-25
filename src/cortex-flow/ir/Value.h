// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/Api.h>
#include <cortex-flow/FlowType.h>

#include <string>
#include <vector>

namespace cortex {
namespace flow {

class Instr;

/**
 * Defines an immutable IR value.
 */
class CORTEX_FLOW_API Value {
 protected:
  Value(const Value& v);

 public:
  Value(FlowType ty, const std::string& name);
  virtual ~Value();

  FlowType type() const { return type_; }
  void setType(FlowType ty) { type_ = ty; }

  const std::string& name() const { return name_; }
  void setName(const std::string& n) { name_ = n; }

  void addUse(Instr* user);
  void removeUse(Instr* user);
  bool isUsed() const { return !uses_.empty(); }
  const std::vector<Instr*>& uses() const { return uses_; }

  /**
   * @brief Replaces all uses of \c this value as operand with value \p newUse
   * instead.
   * @param newUse the new value to be used.
   */
  void replaceAllUsesWith(Value* newUse);

  virtual void dump() = 0;

 private:
  FlowType type_;
  std::string name_;

  std::vector<Instr*> uses_;  //! list of instructions that <b>use</b> this
                              //value.
};

}  // namespace flow
}  // namespace cortex
