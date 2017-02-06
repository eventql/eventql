/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <assert.h>
#include <eventql/sql/stype.h>
#include <eventql/sql/svalue.h>
#include <eventql/sql/runtime/vm.h>

namespace csql {

std::string getSTypeName(SType type) {
  switch (type) {
    case SType::NIL: return "nil";
    case SType::UINT64: return "uint64";
    case SType::INT64: return "int64";
    case SType::FLOAT64: return "float64";
    case SType::BOOL: return "bool";
    case SType::STRING: return "string";
    case SType::TIMESTAMP64: return "timestamp64";
  }

  return "???";
}

void copyBoxed(const SValue* val, SVector* vector) {
  vector->append(val->getData(), val->getSize());
  auto tag = val->getTag();
  vector->append(&tag, sizeof(STag));
}

void popBoxed(VMStack* stack, SValue* value) {
  switch (value->getType()) {

    case SType::NIL:
      return;

    case SType::UINT64:
      popUInt64Boxed(stack, value);
      return;

    case SType::INT64:
      popInt64Boxed(stack, value);
      return;

    case SType::FLOAT64:
      popFloat64Boxed(stack, value);
      return;

    case SType::BOOL:
      popBoolBoxed(stack, value);
      return;

    case SType::STRING:
      popStringBoxed(stack, value);
      return;

    case SType::TIMESTAMP64:
      popTimestamp64Boxed(stack, value);
      return;

  }
}

void popVector(VMStack* stack, SVector* vector) {
  switch (vector->getType()) {

    case SType::NIL:
      return;

    case SType::UINT64:
      popUInt64Vector(stack, vector);
      return;

    case SType::INT64:
      popInt64Vector(stack, vector);
      return;

    case SType::FLOAT64:
      popFloat64Vector(stack, vector);
      return;

    case SType::BOOL:
      popBoolVector(stack, vector);
      return;

    case SType::STRING:
      popStringVector(stack, vector);
      return;

    case SType::TIMESTAMP64:
      popTimestamp64Vector(stack, vector);
      return;

  }
}

void pushBoxed(VMStack* stack, const SValue* value) {
  switch (value->getType()) {

    case SType::NIL:
      return;

    case SType::UINT64:
      pushUInt64Unboxed(stack, value->getData());
      return;

    case SType::INT64:
      pushInt64Unboxed(stack, value->getData());
      return;

    case SType::FLOAT64:
      pushFloat64Unboxed(stack, value->getData());
      return;

    case SType::BOOL:
      pushBoolUnboxed(stack, value->getData());
      return;

    case SType::STRING:
      pushStringUnboxed(stack, value->getData());
      return;

    case SType::TIMESTAMP64:
      pushTimestamp64Unboxed(stack, value->getData());
      return;

  }
}

void pushUnboxed(VMStack* stack, SType type, const void* value) {
  switch (type) {

    case SType::NIL:
      return;

    case SType::UINT64:
      pushUInt64Unboxed(stack, value);
      return;

    case SType::INT64:
      pushInt64Unboxed(stack, value);
      return;

    case SType::FLOAT64:
      pushFloat64Unboxed(stack, value);
      return;

    case SType::BOOL:
      pushBoolUnboxed(stack, value);
      return;

    case SType::STRING:
      pushStringUnboxed(stack, value);
      return;

    case SType::TIMESTAMP64:
      pushTimestamp64Unboxed(stack, value);
      return;

  }
}

void popNil(VMStack* stack) {
  assert(stack->limit - stack->top >= sizeof(STag));
  stack->top += sizeof(STag);
}

void popNilBoxed(VMStack* stack, SValue* value) {
  assert(value->getType() == SType::NIL);
  assert(stack->limit - stack->top >= sizeof(STag));
  STag tag;
  memcpy(&tag, stack->top, sizeof(STag));
  value->setTag(tag);
  stack->top += sizeof(STag);
}

void popNilVector(VMStack* stack, SVector* vector) {
  assert(vector->getType() == SType::NIL);
  assert(stack->limit - stack->top >= sizeof(STag));
  vector->append(stack->top, sizeof(STag));
  stack->top += sizeof(STag);
}

void pushNil(VMStack* stack) {
  if (stack->top - stack->data < sizeof(STag)) {
    vm::growStack(stack, sizeof(STag));
  }

  stack->top -= sizeof(STag);
  memset(stack->top, 0, sizeof(STag));
}

void pushNilUnboxed(VMStack* stack, const void* value) {
  if (stack->top - stack->data < sizeof(STag)) {
    vm::growStack(stack, sizeof(STag));
  }

  stack->top -= sizeof(STag);
  memcpy(stack->top, value, sizeof(STag));
}

uint64_t popUInt64(VMStack* stack) {
  assert(stack->limit - stack->top >= sizeof(uint64_t) + sizeof(STag));
  uint64_t value;
  memcpy(&value, stack->top, sizeof(uint64_t));
  stack->top += sizeof(uint64_t) + sizeof(STag);
  return value;
}

void popUInt64Boxed(VMStack* stack, SValue* value) {
  assert(value->getType() == SType::UINT64);
  assert(stack->limit - stack->top >= sizeof(uint64_t) + sizeof(STag));
  memcpy(value->getData(), stack->top, sizeof(uint64_t));
  STag tag;
  memcpy(&tag, stack->top + sizeof(uint64_t), sizeof(STag));
  value->setTag(tag);
  stack->top += sizeof(uint64_t) + sizeof(STag);
}

void popUInt64Vector(VMStack* stack, SVector* vector) {
  assert(vector->getType() == SType::UINT64);
  assert(stack->limit - stack->top >= sizeof(uint64_t) + sizeof(STag));
  vector->append(stack->top, sizeof(uint64_t) + sizeof(STag));
  stack->top += sizeof(uint64_t) + sizeof(STag);
}

void pushUInt64(VMStack* stack, uint64_t value) {
  if (stack->top - stack->data < sizeof(uint64_t) + sizeof(STag)) {
    vm::growStack(stack, sizeof(uint64_t) + sizeof(STag));
  }

  stack->top -= sizeof(uint64_t) + sizeof(STag);
  memcpy(stack->top, &value, sizeof(uint64_t));
  memset(stack->top + sizeof(uint64_t), 0, sizeof(STag));
}

void pushUInt64Unboxed(VMStack* stack, const void* value) {
  if (stack->top - stack->data < sizeof(uint64_t) + sizeof(STag)) {
    vm::growStack(stack, sizeof(uint64_t) + sizeof(STag));
  }

  stack->top -= sizeof(uint64_t) + sizeof(STag);
  memcpy(stack->top, value, sizeof(uint64_t) + sizeof(STag));
}

int64_t popInt64(VMStack* stack) {
  assert(stack->limit - stack->top >= sizeof(int64_t) + sizeof(STag));
  int64_t value;
  memcpy(&value, stack->top, sizeof(int64_t));
  stack->top += sizeof(int64_t) + sizeof(STag);
  return value;
}

void popInt64Boxed(VMStack* stack, SValue* value) {
  assert(value->getType() == SType::INT64);
  assert(stack->limit - stack->top >= sizeof(int64_t) + sizeof(STag));
  memcpy(value->getData(), stack->top, sizeof(int64_t));
  STag tag;
  memcpy(&tag, stack->top + sizeof(int64_t), sizeof(STag));
  value->setTag(tag);
  stack->top += sizeof(int64_t) + sizeof(STag);
}

void popInt64Vector(VMStack* stack, SVector* vector) {
  assert(vector->getType() == SType::INT64);
  assert(stack->limit - stack->top >= sizeof(int64_t) + sizeof(STag));
  vector->append(stack->top, sizeof(int64_t) + sizeof(STag));
  stack->top += sizeof(int64_t) + sizeof(STag);
}

void pushInt64(VMStack* stack, int64_t value) {
  if (stack->top - stack->data < sizeof(int64_t) + sizeof(STag)) {
    vm::growStack(stack, sizeof(int64_t) + sizeof(STag));
  }

  stack->top -= sizeof(int64_t) + sizeof(STag);
  memcpy(stack->top, &value, sizeof(int64_t));
  memset(stack->top + sizeof(int64_t), 0, sizeof(STag));
}

void pushInt64Unboxed(VMStack* stack, const void* value) {
  if (stack->top - stack->data < sizeof(int64_t) + sizeof(STag)) {
    vm::growStack(stack, sizeof(int64_t) + sizeof(STag));
  }

  stack->top -= sizeof(int64_t) + sizeof(STag);
  memcpy(stack->top, value, sizeof(int64_t) + sizeof(STag));
}

double popFloat64(VMStack* stack) {
  assert(stack->limit - stack->top >= sizeof(double) + sizeof(STag));
  double value;
  memcpy(&value, stack->top, sizeof(double));
  stack->top += sizeof(double) + sizeof(STag);
  return value;
}

void popFloat64Boxed(VMStack* stack, SValue* value) {
  assert(value->getType() == SType::FLOAT64);
  assert(stack->limit - stack->top >= sizeof(double) + sizeof(STag));
  memcpy(value->getData(), stack->top, sizeof(double));
  STag tag;
  memcpy(&tag, stack->top + sizeof(double), sizeof(STag));
  value->setTag(tag);
  stack->top += sizeof(double) + sizeof(STag);
}

void popFloat64Vector(VMStack* stack, SVector* vector) {
  assert(vector->getType() == SType::FLOAT64);
  assert(stack->limit - stack->top >= sizeof(double) + sizeof(STag));
  vector->append(stack->top, sizeof(double) + sizeof(STag));
  stack->top += sizeof(double) + sizeof(STag);
}

void pushFloat64(VMStack* stack, double value) {
  if (stack->top - stack->data < sizeof(double) + sizeof(STag)) {
    vm::growStack(stack, sizeof(double) + sizeof(STag));
  }

  stack->top -= sizeof(double) + sizeof(STag);
  memcpy(stack->top, &value, sizeof(double));
  memset(stack->top + sizeof(double), 0, sizeof(STag));
}

void pushFloat64Unboxed(VMStack* stack, const void* value) {
  if (stack->top - stack->data < sizeof(double) + sizeof(STag)) {
    vm::growStack(stack, sizeof(double) + sizeof(STag));
  }

  stack->top -= sizeof(double) + sizeof(STag);
  memcpy(stack->top, value, sizeof(double) + sizeof(STag));
}

bool popBool(VMStack* stack) {
  assert(stack->limit - stack->top >= sizeof(uint8_t) + sizeof(STag));
  uint8_t value;
  memcpy(&value, stack->top, sizeof(uint8_t));
  stack->top += sizeof(uint8_t) + sizeof(STag);
  return value;
}

void popBoolBoxed(VMStack* stack, SValue* value) {
  assert(value->getType() == SType::BOOL);
  assert(stack->limit - stack->top >= sizeof(uint8_t) + sizeof(STag));
  memcpy(value->getData(), stack->top, sizeof(uint8_t));
  STag tag;
  memcpy(&tag, stack->top + sizeof(uint8_t), sizeof(STag));
  value->setTag(tag);
  stack->top += sizeof(uint8_t) + sizeof(STag);
}

void popBoolVector(VMStack* stack, SVector* vector) {
  assert(vector->getType() == SType::BOOL);
  assert(stack->limit - stack->top >= sizeof(uint8_t) + sizeof(STag));
  vector->append(stack->top, sizeof(uint8_t) + sizeof(STag));
  stack->top += sizeof(uint8_t) + sizeof(STag);
}

void pushBool(VMStack* stack, bool value_bool) {
  if (stack->top - stack->data < sizeof(uint8_t) + sizeof(STag)) {
    vm::growStack(stack, sizeof(uint8_t) + sizeof(STag));
  }

  uint8_t value = value_bool;
  stack->top -= sizeof(uint8_t) + sizeof(STag);
  memcpy(stack->top, &value, sizeof(uint8_t));
  memset(stack->top + sizeof(uint8_t), 0, sizeof(STag));
}

void pushBoolUnboxed(VMStack* stack, const void* value) {
  if (stack->top - stack->data < sizeof(uint8_t) + sizeof(STag)) {
    vm::growStack(stack, sizeof(uint8_t) + sizeof(STag));
  }

  stack->top -= sizeof(uint8_t) + sizeof(STag);
  memcpy(stack->top, value, sizeof(uint8_t) + sizeof(STag));
}

void copyString(const std::string& str, SVector* vector) {
  uint32_t strlen = str.size();
  vector->append((const char*) &strlen, sizeof(uint32_t));
  vector->append(str.data(), strlen);
  STag tag = 0;
  vector->append(&tag, sizeof(tag));
}

void popString(VMStack* stack, const char** data, size_t* len) {
  assert(stack->limit - stack->top >= sizeof(uint32_t));
  *len = *reinterpret_cast<uint32_t*>(stack->top);
  *data = stack->top + sizeof(uint32_t);
  assert(stack->limit - stack->top >= sizeof(uint32_t) + *len + sizeof(STag));
  stack->top += sizeof(uint32_t) + *len + sizeof(STag);
}

std::string popString(VMStack* stack) {
  assert(stack->limit - stack->top >= sizeof(uint32_t));
  size_t len = *reinterpret_cast<uint32_t*>(stack->top);
  auto data = stack->top + sizeof(uint32_t);
  assert(stack->limit - stack->top >= sizeof(uint32_t) + len + sizeof(STag));
  stack->top += sizeof(uint32_t) + len + sizeof(STag);
  return std::string(data, len);
}

void popStringBoxed(VMStack* stack, SValue* value) {
  assert(value->getType() == SType::STRING);
  assert(stack->limit - stack->top >= sizeof(uint32_t));
  size_t len = *reinterpret_cast<uint32_t*>(stack->top);
  assert(stack->limit - stack->top >= sizeof(uint32_t) + len + sizeof(STag));
  value->setData(stack->top, sizeof(uint32_t) + len);
  STag tag;
  memcpy(&tag, stack->top + sizeof(uint32_t) + len, sizeof(STag));
  value->setTag(tag);
  stack->top += sizeof(uint32_t) + len + sizeof(STag);
}

void popStringVector(VMStack* stack, SVector* vector) {
  assert(vector->getType() == SType::STRING);
  assert(stack->limit - stack->top >= sizeof(uint32_t));
  size_t len = *reinterpret_cast<uint32_t*>(stack->top);
  assert(stack->limit - stack->top >= sizeof(uint32_t) + len + sizeof(STag));
  vector->append(stack->top, sizeof(uint32_t) + len + sizeof(STag));
  stack->top += sizeof(uint32_t) + len + sizeof(STag);
}

void pushString(VMStack* stack, const char* data, size_t len) {
  if (stack->top - stack->data < sizeof(uint32_t) + len + sizeof(STag)) {
    vm::growStack(stack, sizeof(uint32_t) + len + sizeof(STag));
  }

  *reinterpret_cast<uint32_t*>(stack->top - len - sizeof(uint32_t) - sizeof(STag)) = len;
  memcpy(stack->top - len - sizeof(STag), data, len);
  STag tag = 0;
  memcpy(stack->top - sizeof(STag), &tag, sizeof(STag));
  stack->top -= sizeof(uint32_t) + len + sizeof(STag);
}

void pushString(VMStack* stack, const std::string& str) {
  auto len = str.size();

  if (stack->top - stack->data < sizeof(uint32_t) + len + sizeof(STag)) {
    vm::growStack(stack, sizeof(uint32_t) + len + sizeof(STag));
  }

  *reinterpret_cast<uint32_t*>(stack->top - sizeof(uint32_t) - len - sizeof(STag)) = len;
  memcpy(stack->top - len - sizeof(STag), str.data(), len);
  STag tag = 0;
  memcpy(stack->top - sizeof(STag), &tag, sizeof(STag));
  stack->top -= sizeof(uint32_t) + len + sizeof(STag);
}

void pushStringUnboxed(VMStack* stack, const void* value) {
  auto len = *reinterpret_cast<const uint32_t*>(value);
  if (stack->top - stack->data < sizeof(uint32_t) + len + sizeof(STag)) {
    vm::growStack(stack, sizeof(uint32_t) + len + sizeof(STag));
  }

  memcpy(
      stack->top - sizeof(uint32_t) - len - sizeof(STag), 
      value,
      len + sizeof(uint32_t) + sizeof(STag));

  stack->top -= sizeof(uint32_t) + len + sizeof(STag);
}

uint64_t popTimestamp64(VMStack* stack) {
  assert(stack->limit - stack->top >= sizeof(uint64_t) + sizeof(STag));
  uint64_t value;
  memcpy(&value, stack->top, sizeof(uint64_t));
  stack->top += sizeof(uint64_t) + sizeof(STag);
  return value;
}

void popTimestamp64Boxed(VMStack* stack, SValue* value) {
  assert(value->getType() == SType::TIMESTAMP64);
  assert(stack->limit - stack->top >= sizeof(uint64_t) + sizeof(STag));
  memcpy(value->getData(), stack->top, sizeof(uint64_t));
  STag tag;
  memcpy(&tag, stack->top + sizeof(uint64_t), sizeof(STag));
  value->setTag(tag);
  stack->top += sizeof(uint64_t) + sizeof(STag);
}

void popTimestamp64Vector(VMStack* stack, SVector* vector) {
  assert(vector->getType() == SType::TIMESTAMP64);
  assert(stack->limit - stack->top >= sizeof(uint64_t) + sizeof(STag));
  vector->append(stack->top, sizeof(uint64_t) + sizeof(STag));
  stack->top += sizeof(uint64_t) + sizeof(STag);
}

void pushTimestamp64(VMStack* stack, uint64_t value) {
  if (stack->top - stack->data < sizeof(uint64_t) + sizeof(STag)) {
    vm::growStack(stack, sizeof(uint64_t) + sizeof(STag));
  }

  stack->top -= sizeof(uint64_t) + sizeof(STag);
  memcpy(stack->top, &value, sizeof(uint64_t));
  memset(stack->top + sizeof(uint64_t), 0, sizeof(STag));
}

void pushTimestamp64Unboxed(VMStack* stack, const void* value) {
  if (stack->top - stack->data < sizeof(uint64_t) + sizeof(STag)) {
    vm::growStack(stack, sizeof(uint64_t) + sizeof(STag));
  }

  stack->top -= sizeof(uint64_t) + sizeof(STag);
  memcpy(stack->top, value, sizeof(uint64_t) + sizeof(STag));
}

} // namespace csql

