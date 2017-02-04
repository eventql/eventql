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

uint64_t popUInt64(VMStack* stack) {
  assert(stack->limit - stack->top >= sizeof(uint64_t));
  auto value = *reinterpret_cast<uint64_t*>(stack->top);
  stack->top += sizeof(uint64_t);
  return value;
}

void popUInt64Boxed(VMStack* stack, SValue* value) {
  assert(value->getType() == SType::UINT64);
  assert(stack->limit - stack->top >= sizeof(uint64_t));
  memcpy(value->getData(), stack->top, sizeof(uint64_t));
  stack->top += sizeof(uint64_t);
}

void popUInt64Vector(VMStack* stack, SVector* vector) {
  assert(vector->getType() == SType::UINT64);
  assert(stack->limit - stack->top >= sizeof(uint64_t));
  vector->append(stack->top, sizeof(uint64_t));
  stack->top += sizeof(uint64_t);
}

void pushUInt64(VMStack* stack, uint64_t value) {
  if (stack->top - stack->data < sizeof(uint64_t)) {
    vm::growStack(stack, sizeof(uint64_t));
  }

  stack->top -= sizeof(uint64_t);
  *reinterpret_cast<uint64_t*>(stack->top) = value;
}

void pushUInt64Unboxed(VMStack* stack, const void* value) {
  if (stack->top - stack->data < sizeof(uint64_t)) {
    vm::growStack(stack, sizeof(uint64_t));
  }

  stack->top -= sizeof(uint64_t);
  memcpy(stack->top, value, sizeof(uint64_t));
}

int64_t popInt64(VMStack* stack) {
  assert(stack->limit - stack->top >= sizeof(int64_t));
  auto value = *reinterpret_cast<int64_t*>(stack->top);
  stack->top += sizeof(int64_t);
  return value;
}

void popInt64Boxed(VMStack* stack, SValue* value) {
  assert(value->getType() == SType::INT64);
  assert(stack->limit - stack->top >= sizeof(int64_t));
  memcpy(value->getData(), stack->top, sizeof(int64_t));
  stack->top += sizeof(int64_t);
}

void popInt64Vector(VMStack* stack, SVector* vector) {
  assert(vector->getType() == SType::INT64);
  assert(stack->limit - stack->top >= sizeof(int64_t));
  vector->append(stack->top, sizeof(int64_t));
  stack->top += sizeof(int64_t);
}

void pushInt64(VMStack* stack, int64_t value) {
  if (stack->top - stack->data < sizeof(int64_t)) {
    vm::growStack(stack, sizeof(int64_t));
  }

  stack->top -= sizeof(int64_t);
  *reinterpret_cast<int64_t*>(stack->top) = value;
}

void pushInt64Unboxed(VMStack* stack, const void* value) {
  if (stack->top - stack->data < sizeof(int64_t)) {
    vm::growStack(stack, sizeof(int64_t));
  }

  stack->top -= sizeof(int64_t);
  memcpy(stack->top, value, sizeof(int64_t));
}

double popFloat64(VMStack* stack) {
  assert(stack->limit - stack->top >= sizeof(double));
  auto value = *reinterpret_cast<double*>(stack->top);
  stack->top += sizeof(double);
  return value;
}

void popFloat64Boxed(VMStack* stack, SValue* value) {
  assert(value->getType() == SType::FLOAT64);
  assert(stack->limit - stack->top >= sizeof(double));
  memcpy(value->getData(), stack->top, sizeof(double));
  stack->top += sizeof(double);
}

void popFloat64Vector(VMStack* stack, SVector* vector) {
  assert(vector->getType() == SType::FLOAT64);
  assert(stack->limit - stack->top >= sizeof(double));
  vector->append(stack->top, sizeof(double));
  stack->top += sizeof(double);
}

void pushFloat64(VMStack* stack, double value) {
  if (stack->top - stack->data < sizeof(double)) {
    vm::growStack(stack, sizeof(double));
  }

  stack->top -= sizeof(double);
  *reinterpret_cast<double*>(stack->top) = value;
}

void pushFloat64Unboxed(VMStack* stack, const void* value) {
  if (stack->top - stack->data < sizeof(double)) {
    vm::growStack(stack, sizeof(double));
  }

  stack->top -= sizeof(double);
  memcpy(stack->top, value, sizeof(double));
}

bool popBool(VMStack* stack) {
  assert(stack->limit - stack->top >= sizeof(uint8_t));
  auto value = *reinterpret_cast<uint8_t*>(stack->top);
  stack->top += sizeof(uint8_t);
  return value;
}

void popBoolBoxed(VMStack* stack, SValue* value) {
  assert(value->getType() == SType::BOOL);
  assert(stack->limit - stack->top >= sizeof(uint8_t));
  memcpy(value->getData(), stack->top, sizeof(uint8_t));
  stack->top += sizeof(uint8_t);
}

void popBoolVector(VMStack* stack, SVector* vector) {
  assert(vector->getType() == SType::BOOL);
  assert(stack->limit - stack->top >= sizeof(uint8_t));
  vector->append(stack->top, sizeof(uint8_t));
  stack->top += sizeof(uint8_t);
}

void pushBool(VMStack* stack, bool value) {
  if (stack->top - stack->data < sizeof(uint8_t)) {
    vm::growStack(stack, sizeof(uint8_t));
  }

  stack->top -= sizeof(uint8_t);
  *reinterpret_cast<uint8_t*>(stack->top) = uint8_t(value);
}

void pushBoolUnboxed(VMStack* stack, const void* value) {
  if (stack->top - stack->data < sizeof(uint8_t)) {
    vm::growStack(stack, sizeof(uint8_t));
  }

  stack->top -= sizeof(uint8_t);
  memcpy(stack->top, value, sizeof(uint8_t));
}

void popString(VMStack* stack, const char** data, size_t* len) {
  assert(stack->limit - stack->top >= sizeof(uint32_t));
  *len = *reinterpret_cast<uint32_t*>(stack->top);
  *data = stack->top + sizeof(uint32_t);
  assert(stack->limit - stack->top >= sizeof(uint32_t) + *len);
  stack->top += sizeof(uint32_t) + *len;
}

std::string popString(VMStack* stack) {
  assert(stack->limit - stack->top >= sizeof(uint32_t));
  size_t len = *reinterpret_cast<uint32_t*>(stack->top);
  auto data = stack->top + sizeof(uint32_t);
  assert(stack->limit - stack->top >= sizeof(uint32_t) + len);
  stack->top += len + sizeof(uint32_t);
  return std::string(data, len);
}

void popStringBoxed(VMStack* stack, SValue* value) {
  assert(stack->limit - stack->top >= sizeof(uint32_t));
  size_t len = *reinterpret_cast<uint32_t*>(stack->top);
  value->setData(stack->top, sizeof(uint32_t) + len);
  assert(stack->limit - stack->top >= sizeof(uint32_t) + len);
  stack->top += sizeof(uint32_t) + len;
}

void popStringVector(VMStack* stack, SVector* vector) {
  assert(stack->limit - stack->top >= sizeof(uint32_t));
  size_t len = *reinterpret_cast<uint32_t*>(stack->top);
  vector->append(stack->top, sizeof(uint32_t) + len);
  assert(stack->limit - stack->top >= sizeof(uint32_t) + len);
  stack->top += sizeof(uint32_t) + len;
}

void pushString(VMStack* stack, const char* data, size_t len) {
  if (stack->top - stack->data < sizeof(uint32_t) + len) {
    vm::growStack(stack, sizeof(uint32_t) + len);
  }

  *reinterpret_cast<uint32_t*>(stack->top - len - sizeof(uint32_t)) = len;
  memcpy(stack->top - len, data, len);
  stack->top -= sizeof(uint32_t) + len;
}

void pushString(VMStack* stack, const std::string& str) {
  auto len = str.size();

  if (stack->top - stack->data < sizeof(uint32_t) + len) {
    vm::growStack(stack, sizeof(uint32_t) + len);
  }

  *reinterpret_cast<uint32_t*>(stack->top - sizeof(uint32_t) - len) = len;
  memcpy(stack->top - len, str.data(), len);
  stack->top -= sizeof(uint32_t) + len;
}

void pushStringUnboxed(VMStack* stack, const void* value) {
  auto len = *static_cast<const uint32_t*>(value);
  if (stack->top - stack->data < sizeof(uint32_t) + len) {
    vm::growStack(stack, sizeof(uint32_t) + len);
  }

  memcpy(stack->top - sizeof(uint32_t) - len, value, len + sizeof(uint32_t));
  stack->top -= sizeof(uint32_t) + len;
}

uint64_t popTimestamp64(VMStack* stack) {
  assert(stack->limit - stack->top >= sizeof(uint64_t));
  auto value = *reinterpret_cast<uint64_t*>(stack->top);
  stack->top += sizeof(uint64_t);
  return value;
}

void popTimestamp64Boxed(VMStack* stack, SValue* value) {
  assert(value->getType() == SType::UINT64);
  assert(stack->limit - stack->top >= sizeof(uint64_t));
  memcpy(value->getData(), stack->top, sizeof(uint64_t));
  stack->top += sizeof(uint64_t);
}

void popTimestamp64Vector(VMStack* stack, SVector* vector) {
  assert(vector->getType() == SType::UINT64);
  assert(stack->limit - stack->top >= sizeof(uint64_t));
  vector->append(stack->top, sizeof(uint64_t));
  stack->top += sizeof(uint64_t);
}

void pushTimestamp64(VMStack* stack, uint64_t value) {
  if (stack->top - stack->data < sizeof(uint64_t)) {
    vm::growStack(stack, sizeof(uint64_t));
  }

  stack->top -= sizeof(uint64_t);
  *reinterpret_cast<uint64_t*>(stack->top) = value;
}

void pushTimestamp64Unboxed(VMStack* stack, const void* value) {
  if (stack->top - stack->data < sizeof(uint64_t)) {
    vm::growStack(stack, sizeof(uint64_t));
  }

  stack->top -= sizeof(uint64_t);
  memcpy(stack->top, value, sizeof(uint64_t));
}

} // namespace csql

