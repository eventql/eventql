/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>
#include <stx/io/inputstream.h>
#include <stx/io/outputstream.h>
#include <stx/stdtypes.h>
#include <stx/UnixTime.h>
#include <stx/exception.h>
#include <csql/csql.h>

using namespace stx;

namespace csql {

class SValue {
public:
  typedef std::string StringType;
  typedef double FloatType;
  typedef int64_t IntegerType;
  typedef bool BoolType;
  typedef stx::UnixTime TimeType;

  static SValue newNull();
  static SValue newString(const String& value);
  static SValue newString(const char* value);
  static SValue newInteger(IntegerType value);
  static SValue newInteger(const String& value);
  static SValue newFloat(FloatType value);
  static SValue newFloat(const String& value);
  static SValue newBool(BoolType value);
  static SValue newBool(const String& value);
  static SValue newTimestamp(TimeType value);
  static SValue newTimestamp(const String& value);

  static const char* getTypeName(sql_type type);
  const char* getTypeName() const;

  explicit SValue();
  SValue(const SValue& copy);
  SValue& operator=(const SValue& copy);
  bool operator==(const SValue& other) const;
  ~SValue();

  // deprecated constructors
  explicit SValue(const StringType& string_value);
  explicit SValue(char const* string_value); // FIXPAUL HACK!!!
  explicit SValue(IntegerType integer_value);
  explicit SValue(FloatType float_value);
  explicit SValue(BoolType bool_value);
  explicit SValue(TimeType time_value);

  sql_type getType() const;
  bool isString() const;
  bool isNumeric() const;
  bool isInteger() const;
  bool isFloat() const;
  bool isBool() const;
  bool isTimestamp() const;

  template <typename T> T getValue() const;
  StringType getString() const;
  IntegerType getInteger() const;
  FloatType getFloat() const;
  BoolType getBool() const;
  TimeType getTimestamp() const;

  template <typename T> bool isConvertibleTo() const;
  bool isConvertibleToString() const;
  bool isConvertibleToNumeric() const;
  bool isConvertibleToInteger() const;
  bool isConvertibleToFloat() const;
  bool isConvertibleToBool() const;
  bool isConvertibleToTimestamp() const;

  SValue toNumeric() const;
  SValue toString() const;
  SValue toInteger() const;
  SValue toFloat() const;
  SValue toBool() const;
  SValue toTimestamp() const;

  void encode(OutputStream* os) const;
  void decode(InputStream* is);

  String toSQL() const;

  static std::string makeUniqueKey(SValue* arr, size_t len);

protected:
  struct {
    sql_type type;
    union {
      int64_t t_integer;
      double t_float;
      bool t_bool;
      uint64_t t_timestamp;
      struct {
        char* ptr;
        uint32_t len;
      } t_string;
    } u;
  } data_;
};

String sql_escape(const String& str);

}

namespace std {

template <>
struct hash<csql::SValue> {
  size_t operator()(const csql::SValue& sval) const;
};

}
