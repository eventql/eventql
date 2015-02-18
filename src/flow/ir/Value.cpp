// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/ir/Value.h>
#include <xzero/flow/ir/Instr.h>
#include <algorithm>
#include <assert.h>

namespace xzero {
namespace flow {

static unsigned long long valueCounter = 1;

Value::Value(const Value& v) : type_(v.type_), name_(), uses_() {
  char buf[256];
  snprintf(buf, sizeof(buf), "%s%llu", v.name().c_str(), valueCounter);
  valueCounter++;
  name_ = buf;
}

Value::Value(FlowType ty, const std::string& name)
    : type_(ty), name_(name), uses_() {
  if (name_.empty()) {
    char buf[256];
    snprintf(buf, sizeof(buf), "unnamed%llu", valueCounter);
    valueCounter++;
    name_ = buf;
    // printf("default-create name: %s\n", name_.c_str());
  }
}

Value::~Value() {
  assert(!isUsed() && "Value being destroyed is still in use.");
}

void Value::addUse(Instr* user) { uses_.push_back(user); }

void Value::removeUse(Instr* user) {
  auto i = std::find(uses_.begin(), uses_.end(), user);

  assert(i != uses_.end());

  if (i != uses_.end()) {
    uses_.erase(i);
  }
}

void Value::replaceAllUsesWith(Value* newUse) {
  auto myUsers = uses_;

  for (Instr* user : myUsers) {
    user->replaceOperand(this, newUse);
  }
}

void Value::dump() {
  printf("Value '%s': %s\n", name_.c_str(), tos(type_).c_str());
}

}  // namespace flow
}  // namespace xzero
