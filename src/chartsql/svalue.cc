/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>
#include <string>
#include <ctime>
#include <stdint.h>
#include <fnord/inspect.h>
#include <chartsql/svalue.h>
#include <chartsql/format.h>
#include <chartsql/parser/token.h>

namespace csql {

SValue::SValue() {
  memset(&data_, 0, sizeof(data_));
  data_.type = T_NULL;
}

SValue::~SValue() {
  // FIXPAUL free string!
}

SValue::SValue(const SValue::StringType& string_value) {
  data_.type = T_STRING;
  data_.u.t_string.len = string_value.size();
  data_.u.t_string.ptr = static_cast<char *>(malloc(data_.u.t_string.len));

  if (data_.u.t_string.ptr == nullptr) {
    RAISE(kRuntimeError, "could not allocate SValue");
  }

  memcpy(
      data_.u.t_string.ptr,
      string_value.c_str(),
      data_.u.t_string.len);
}

SValue::SValue(
    char const* string_value) :
    SValue(std::string(string_value)) {}

SValue::SValue(SValue::IntegerType integer_value) {
  data_.type = T_INTEGER;
  data_.u.t_integer = integer_value;
}

SValue::SValue(SValue::FloatType float_value) {
  data_.type = T_FLOAT;
  data_.u.t_float = float_value;
}

SValue::SValue(SValue::BoolType bool_value) {
  data_.type = T_BOOL;
  data_.u.t_bool = bool_value;
}

SValue::SValue(SValue::TimeType time_value) {
  data_.type = T_TIMESTAMP;
  data_.u.t_timestamp = static_cast<uint64_t>(time_value);
}

SValue::SValue(const SValue& copy) {
  switch (copy.data_.type) {

    case T_STRING:
      data_.type = T_STRING;
      data_.u.t_string.len = copy.data_.u.t_string.len;
      data_.u.t_string.ptr = static_cast<char *>(malloc(data_.u.t_string.len));

      if (data_.u.t_string.ptr == nullptr) {
        RAISE(kRuntimeError, "could not allocate SValue");
      }

      memcpy(
          data_.u.t_string.ptr,
          copy.data_.u.t_string.ptr,
          data_.u.t_string.len);

    default:
      memcpy(&data_, &copy.data_, sizeof(data_));
      break;

  }

}

SValue& SValue::operator=(const SValue& copy) {
  // FIXPAUL free old string!

  switch (copy.data_.type) {

    case T_STRING:
      data_.type = T_STRING;
      data_.u.t_string.len = copy.data_.u.t_string.len;
      data_.u.t_string.ptr = static_cast<char *>(malloc(data_.u.t_string.len));

      if (data_.u.t_string.ptr == nullptr) {
        RAISE(kRuntimeError, "could not allocate SValue");
      }

      memcpy(
          data_.u.t_string.ptr,
          copy.data_.u.t_string.ptr,
          data_.u.t_string.len);

    default:
      memcpy(&data_, &copy.data_, sizeof(data_));
      break;

  }

  return *this;
}

bool SValue::operator==(const SValue& other) const {
  if (data_.type != other.data_.type) {
    return false;
  }

  switch (data_.type) {
    case T_STRING:
      return memcmp(
          data_.u.t_string.ptr,
          other.data_.u.t_string.ptr,
          data_.u.t_string.len) == 0;

    default:
      return memcmp(&data_, &other.data_, sizeof(data_)) == 0;

  }
}

SValue::kSValueType SValue::getType() const {
  return data_.type;
}

SValue::IntegerType SValue::getInteger() const {
  switch (data_.type) {

    case T_INTEGER:
      return data_.u.t_integer;

    case T_TIMESTAMP:
      return data_.u.t_timestamp;

    case T_STRING:
      try {
        return std::stol(getString());
      } catch (std::exception e) {
        /* fallthrough */
      }

    default:
      RAISE(
          kTypeError,
          "can't convert %s '%s' to Integer",
          SValue::getTypeName(data_.type),
          toString().c_str());

  }

  return 0;
}

SValue::FloatType SValue::getFloat() const {
  switch (data_.type) {

    case T_INTEGER:
      return data_.u.t_integer;

    case T_FLOAT:
      return data_.u.t_float;

    case T_STRING:
      try {
        return std::stod(getString());
      } catch (std::exception e) {
        /* fallthrough */
      }

    default:
      RAISE(
          kTypeError,
          "can't convert %s '%s' to Float",
          SValue::getTypeName(data_.type),
          toString().c_str());

  }

  return 0;
}

SValue::BoolType SValue::getBool() const {
  if (data_.type != T_BOOL) {
    RAISE(
       kTypeError,
        "can't convert %s '%s' to Bool",
        SValue::getTypeName(data_.type),
        toString().c_str());
  }

  return data_.u.t_bool;
}

SValue::BoolType SValue::getBoolWithConversion() const {
  switch (data_.type) {

    case T_INTEGER:
      return getInteger() > 0;

    case T_FLOAT:
      return getFloat() > 0;

    case T_BOOL:
      return getBool();

    case T_STRING:
      return true;

    case T_NULL:
      return false;

    default:
      RAISEF(
         kTypeError,
          "can't convert $0 '$1' to Boolean",
          SValue::getTypeName(data_.type),
          toString());

  }
}

SValue::TimeType SValue::getTimestamp() const {
  switch (getType()) {

    case T_TIMESTAMP:
      return data_.u.t_timestamp;

    default:
      RAISE(
         kTypeError,
          "can't convert %s '%s' to Timestamp",
          SValue::getTypeName(data_.type),
          toString().c_str());

  }
}

SValue::StringType SValue::getString() const {
  if (data_.type == T_STRING) {
    return std::string(data_.u.t_string.ptr, data_.u.t_string.len);
  } else {
    return toString();
  }
}

std::string SValue::makeUniqueKey(SValue* arr, size_t len) {
  std::string key;

  for (int i = 0; i < len; ++i) {
    key.append(arr[i].toString());
    key.append("\x00");
  }

  return key;
}

std::string SValue::toString() const {
  char buf[512];
  const char* str;
  size_t len;

  switch (data_.type) {

    case T_INTEGER: {
      len = snprintf(buf, sizeof(buf), "%" PRId64, getInteger());
      str = buf;
      break;
    }

    case T_TIMESTAMP: {
      return getTimestamp().toString("%Y-%m-%d %H:%M:%S");
    }

    case T_FLOAT: {
      len = snprintf(buf, sizeof(buf), "%f", getFloat());
      str = buf;
      break;
    }

    case T_BOOL: {
      static const auto true_str = "true";
      static const auto false_str = "false";
      if (getBool()) {
        str = true_str;
        len = strlen(true_str);
      } else {
        str = false_str;
        len = strlen(false_str);
      }
      break;
    }

    case T_STRING: {
      return getString();
    }

    case T_NULL: {
      static const char undef_str[] = "NULL";
      str = undef_str;
      len = sizeof(undef_str) - 1;
    }

  }

  return std::string(str, len);
}

const char* SValue::getTypeName(kSValueType type) {
  switch (type) {
    case T_STRING:
      return "String";
    case T_FLOAT:
      return "Float";
    case T_INTEGER:
      return "Integer";
    case T_BOOL:
      return "Boolean";
    case T_TIMESTAMP:
      return "Timestamp";
    case T_NULL:
      return "NULL";
  }
}

const char* SValue::getTypeName() const {
  return SValue::getTypeName(data_.type);
}

template <> SValue::BoolType SValue::getValue<SValue::BoolType>() const {
  return getBool();
}

template <> SValue::IntegerType SValue::getValue<SValue::IntegerType>() const {
  return getInteger();
}

template <> SValue::FloatType SValue::getValue<SValue::FloatType>() const {
  return getFloat();
}

template <> SValue::StringType SValue::getValue<SValue::StringType>() const {
  return toString();
}

template <> SValue::TimeType SValue::getValue<SValue::TimeType>() const {
  return getTimestamp();
}

// FIXPAUL: smarter type detection
template <> bool SValue::testType<SValue::BoolType>() const {
  return data_.type == T_BOOL;
}

template <> bool SValue::testType<SValue::TimeType>() const {
  return data_.type == T_TIMESTAMP;
}

template <> bool SValue::testType<SValue::IntegerType>() const {
  if (data_.type == T_INTEGER) {
    return true;
  }

  auto str = toString();
  const char* cur = str.c_str();
  const char* end = cur + str.size();

  if (*cur == '-') {
    ++cur;
  }

  if (cur == end) {
    return false;
  }

  for (; cur < end; ++cur) {
    if (*cur < '0' || *cur > '9') {
      return false;
    }
  }

  return true;
}

template <> bool SValue::testType<SValue::FloatType>() const {
  if (data_.type == T_FLOAT) {
    return true;
  }

  auto str = toString();
  bool dot = false;
  const char* c = str.c_str();

  if (*c == '-') {
    ++c;
  }

  for (; *c != 0; ++c) {
    if (*c >= '0' && *c <= '9') {
      continue;
    }

    if (*c == '.' || *c == ',') {
      if (dot) {
        return false;
      } else {
        dot = true;
      }
      continue;
    }

    return false;
  }

  return true;
}

template <> bool SValue::testType<std::string>() const {
  return true;
}

SValue::kSValueType SValue::testTypeWithNumericConversion() const {
  if (testType<SValue::IntegerType>()) return T_INTEGER;
  if (testType<SValue::FloatType>()) return T_FLOAT;
  return getType();
}

bool SValue::tryNumericConversion() {
  if (testType<SValue::IntegerType>()) {
    SValue::IntegerType val = getValue<SValue::IntegerType>();
    data_.type = T_INTEGER;
    data_.u.t_integer = val;
    return true;
  }

  if (testType<SValue::FloatType>()) {
    SValue::FloatType val = getValue<SValue::FloatType>();
    data_.type = T_FLOAT;
    data_.u.t_float = val;
    return true;
  }

  return false;
}

bool SValue::tryTimeConversion() {
  if (!tryNumericConversion()) {
    return false;
  }

  time_t ts = getInteger();
  data_.type = T_TIMESTAMP;
  // FIXPAUL take a smart guess if this is milli, micro, etc
  data_.u.t_timestamp = ts * 1000000llu;
  return true;
}

}

namespace fnord {

template <>
std::string inspect<csql::SValue::kSValueType>(
    const csql::SValue::kSValueType& type) {
  return csql::SValue::getTypeName(type);
}

template <>
std::string inspect<csql::SValue>(
    const csql::SValue& sval) {
  return sval.toString();
}

}

namespace std {

size_t hash<csql::SValue>::operator()(const csql::SValue& sval) const {
  return hash<std::string>()(sval.toString()); // FIXPAUL
}

}
