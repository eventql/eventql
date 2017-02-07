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
#include <eventql/sql/stype.h>

namespace csql {

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

std::string sql_tostring(SType type, const void* value);
std::string sql_toexprstring(SType type, const void* value);
std::string sql_escape(const std::string& str);


} // namespace csql

