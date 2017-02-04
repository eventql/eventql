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
#pragma once
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>
#include "eventql/eventql.h"
#include <eventql/util/io/inputstream.h>
#include <eventql/util/io/outputstream.h>
#include <eventql/util/stdtypes.h>
#include <eventql/util/UnixTime.h>
#include <eventql/util/exception.h>
#include <eventql/sql/csql.h>

namespace csql {
class VMStack;
class SValue;
class SVector;

enum class SType : uint8_t {
  NIL,
  UINT64,
  INT64,
  FLOAT64,
  BOOL,
  STRING,
  TIMESTAMP64
};

std::string getSTypeName(SType type);

void popBoxed(VMStack* stack, SValue* value);
void popVector(VMStack* stack, SVector* vector);
void pushBoxed(VMStack* stack, const SValue* value);
void pushUnboxed(VMStack* stack, SType type, void* value);

uint64_t popUInt64(VMStack* stack);
void popUInt64Boxed(VMStack* stack, SValue* value);
void popUInt64Vector(VMStack* stack, SVector* vector);
void pushUInt64(VMStack* stack, uint64_t value);
void pushUInt64Boxed(VMStack* stack, const SValue* value);
void pushUInt64Unboxed(VMStack* stack, void* value);

int64_t popInt64(VMStack* stack);
void popInt64Boxed(VMStack* stack, SValue* value);
void popInt64Vector(VMStack* stack, SVector* vector);
void pushInt64(VMStack* stack, int64_t value);
void pushInt64Boxed(VMStack* stack, const SValue* value);
void pushInt64Unboxed(VMStack* stack, void* value);

double popFloat64(VMStack* stack);
void pushFloat64(VMStack* stack, double value);

bool popBool(VMStack* stack);
void pushBool(VMStack* stack, bool value);

void popString(VMStack* stack, char** data, size_t* len);
std::string popString(VMStack* stack);
void pushString(VMStack* stack, const char* data, size_t len);
void pushString(VMStack* stack, const std::string& str);

uint64_t popTimestamp64(VMStack* stack);
void pushTimestamp64(VMStack* stack, uint64_t value);

} // namespace csql

