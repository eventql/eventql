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
#include <eventql/util/SHA1.h>
#include <eventql/sql/csql.h>

namespace csql {
struct VMStack;

enum class SType : uint8_t {
  NIL,
  UINT64,
  INT64,
  FLOAT64,
  BOOL,
  STRING,
  TIMESTAMP64
};

using STag = uint8_t;

enum STagFlag : uint8_t {
  STAG_NULL = 1,
  STAG_INLINE = 128
};

class SValue {
public:

  typedef std::string StringType;
  typedef double FloatType;
  typedef uint64_t IntegerType;
  typedef UnixTime TimeType;

  static const size_t kInlineDataSize = 16;
  enum kDeferInitialization { DEFER_INITIALIZATION };

  static SValue newNull();
  static SValue newUInt64(uint64_t value);
  static SValue newUInt64(const std::string& value);
  static SValue newInt64(int64_t value);
  static SValue newInt64(const std::string& value);
  static SValue newFloat64(double value);
  static SValue newFloat64(const std::string& value);
  static SValue newBool(bool value);
  static SValue newBool(const std::string& value);
  static SValue newString(const std::string& value);
  static SValue newString(const char* value, uint32_t value_len);
  static SValue newTimestamp64(uint64_t value);
  static SValue newTimestamp64(const std::string& value);

  SValue();
  SValue(SType type);
  SValue(SType type, kDeferInitialization _);
  SValue(const SValue& copy);
  ~SValue();

  SValue& operator=(const SValue& copy);
  bool operator==(const SValue& other) const;

  SType getType() const;
  bool isUInt64() const;
  bool isInt64() const;
  bool isFloat64() const;
  bool isBool() const;
  bool isString() const;
  bool isTimestamp64() const;

  void setData(const void* data, size_t size);
  const void* getData() const;
  void* getData();

  size_t getSize() const;

  STag getTag() const;
  void setTag(STag tag);

  uint64_t getUInt64() const;
  int64_t getInt64() const;
  double getFloat64() const;
  bool getBool() const;

  std::string toString() const;

  void copyFrom(const void* data);

  void encode(OutputStream* os) const;
  void decode(InputStream* is);

protected:
  SType type_;
  unsigned char data_[kInlineDataSize];
};

class SVector {
public:

  SVector(SType type);
  SVector(const SVector& other) = delete;
  SVector(SVector&& other);
  SVector& operator=(const SVector& other) = delete;
  ~SVector();

  SType getType() const;

  const void* getData() const;
  void* getMutableData();

  size_t getSize() const;
  void setSize(size_t new_size);
  void clear();

  size_t getCapacity() const;
  void increaseCapacity(size_t min_capacity);

  void copyFrom(const SVector* other);
  void append(const void* data, size_t size);
  void append(const SValue& svalue);

  static size_t next(SType type, void** cursor);
  size_t next(void** cursor) const;

protected:
  SType type_;
  void* data_;
  size_t capacity_;
  size_t size_;
};

size_t sql_strlen(const void* str);
char* sql_cstr(void* str);
const char* sql_cstr(const void* str);

size_t sql_sizeof(SType type, const void* value);
size_t sql_sizeof_static(SType type);
size_t sql_sizeof_tuple(const char* data, const SType* val_types, size_t val_cnt);

std::string sql_typename(SType type);
std::string sql_tostring(SType type, const void* value);
std::string sql_toexprstring(SType type, const void* value);
std::string sql_escape(const std::string& str);

void copyBoxed(const SValue* val, SVector* vector);

void popBoxed(VMStack* stack, SValue* value);
void popVector(VMStack* stack, SVector* vector);
void pushBoxed(VMStack* stack, const SValue* value);
void pushUnboxed(VMStack* stack, SType type, const void* value);

void popNil(VMStack* stack);
void popNil(STag* tag);
void popNilBoxed(VMStack* stack, SValue* value);
void popNilVector(VMStack* stack, SVector* vector);
void pushNil(VMStack* stack);
void pushNil(VMStack* stack, STag tag);
void pushNilUnboxed(VMStack* stack, const void* value);

uint64_t popUInt64(VMStack* stack);
void popUInt64(VMStack* stack, uint64_t* value, STag* tag);
void popUInt64Boxed(VMStack* stack, SValue* value);
void popUInt64Vector(VMStack* stack, SVector* vector);
void pushUInt64(VMStack* stack, uint64_t value);
void pushUInt64(VMStack* stack, uint64_t value, STag tag);
void pushUInt64Unboxed(VMStack* stack, const void* value);

int64_t popInt64(VMStack* stack);
void popUInt64(VMStack* stack, int64_t* value, STag* tag);
void popInt64Boxed(VMStack* stack, SValue* value);
void popInt64Vector(VMStack* stack, SVector* vector);
void pushInt64(VMStack* stack, int64_t value);
void pushInt64(VMStack* stack, int64_t value, STag tag);
void pushInt64Unboxed(VMStack* stack, const void* value);

double popFloat64(VMStack* stack);
double popFloat64(VMStack* stack, double* value, STag* tag);
void popFloat64Boxed(VMStack* stack, SValue* value);
void popFloat64Vector(VMStack* stack, SVector* vector);
void pushFloat64(VMStack* stack, double value);
void pushFloat64(VMStack* stack, double value, STag tag);
void pushFloat64Unboxed(VMStack* stack, const void* value);

bool popBool(VMStack* stack);
void popBool(VMStack* stack, bool* value, STag* tag);
void popBoolBoxed(VMStack* stack, SValue* value);
void popBoolVector(VMStack* stack, SVector* vector);
void pushBool(VMStack* stack, bool value);
void pushBool(VMStack* stack, bool value, STag tag);
void pushBoolUnboxed(VMStack* stack, const void* value);

void copyString(const std::string& str, SVector* vector);
void copyString(const char* str, uint32_t strlen, SVector* vector);
void popString(VMStack* stack, const char** data, size_t* len);
void popString(VMStack* stack, const char** data, size_t* len, STag* tag);
std::string popString(VMStack* stack);
void popStringBoxed(VMStack* stack, SValue* value);
void popStringVector(VMStack* stack, SVector* vector);
void pushString(VMStack* stack, const char* data, size_t len);
void pushString(VMStack* stack, const char* data, size_t len, STag tag);
void pushString(VMStack* stack, const std::string& str);
void pushStringUnboxed(VMStack* stack, const void* value);

uint64_t popTimestamp64(VMStack* stack);
void popTimestamp64(VMStack* stack, uint64_t* value, STag* tag);
void popTimestamp64Boxed(VMStack* stack, SValue* value);
void popTimestamp64Vector(VMStack* stack, SVector* vector);
void pushTimestamp64(VMStack* stack, uint64_t value);
void pushTimestamp64(VMStack* stack, uint64_t value, STag tag);
void pushTimestamp64Unboxed(VMStack* stack, const void* value);


} // namespace csql

