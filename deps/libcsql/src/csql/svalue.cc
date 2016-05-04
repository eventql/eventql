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
#include <stx/inspect.h>
#include <stx/human.h>
#include <csql/svalue.h>
#include <csql/format.h>
#include <csql/parser/token.h>

namespace csql {

SValue SValue::newNull() {
  return SValue();
}

SValue SValue::newString(const String& value) {
  return SValue(value);
}

SValue SValue::newString(const char* value) {
  return SValue(value);
}

SValue SValue::newInteger(IntegerType value) {
  return SValue(SValue::IntegerType(value));
}

SValue SValue::newInteger(const String& value) {
  return SValue(value).toInteger();
}

SValue SValue::newFloat(FloatType value) {
  return SValue(value);
}

SValue SValue::newFloat(const String& value) {
  return SValue(value).toFloat();
}

SValue SValue::newBool(BoolType value) {
  return SValue(SValue::BoolType(value));
}

SValue SValue::newBool(const String& value) {
  return SValue(value).toBool();
}

SValue SValue::newTimestamp(TimeType value) {
  return SValue(value);
}

SValue SValue::newTimestamp(const String& value) {
  return SValue(value).toTimestamp();
}

SValue::SValue() {
  memset(&data_, 0, sizeof(data_));
  data_.type = SQL_NULL;
}

SValue::~SValue() {
  switch (data_.type) {

    case SQL_STRING:
      free(data_.u.t_string.ptr);
      break;

    default:
      break;

  }
}

SValue::SValue(const SValue::StringType& string_value) {
  data_.type = SQL_STRING;
  data_.u.t_string.len = string_value.size();
  data_.u.t_string.ptr = static_cast<char *>(malloc(data_.u.t_string.len));

  if (data_.u.t_string.ptr == nullptr) {
    RAISE(kRuntimeError, "could not allocate SValue");
  }

  memcpy(
      data_.u.t_string.ptr,
      string_value.data(),
      data_.u.t_string.len);
}

SValue::SValue(
    char const* string_value) :
    SValue(std::string(string_value)) {}

SValue::SValue(SValue::IntegerType integer_value) {
  data_.type = SQL_INTEGER;
  data_.u.t_integer = integer_value;
}

SValue::SValue(SValue::FloatType float_value) {
  data_.type = SQL_FLOAT;
  data_.u.t_float = float_value;
}

SValue::SValue(SValue::BoolType bool_value) {
  data_.type = SQL_BOOL;
  data_.u.t_bool = bool_value;
}

SValue::SValue(SValue::TimeType time_value) {
  data_.type = SQL_TIMESTAMP;
  data_.u.t_timestamp = static_cast<uint64_t>(time_value);
}

SValue::SValue(const SValue& copy) {
  switch (copy.data_.type) {

    case SQL_STRING:
      data_.type = SQL_STRING;
      data_.u.t_string.len = copy.data_.u.t_string.len;
      data_.u.t_string.ptr = static_cast<char *>(malloc(data_.u.t_string.len));

      if (data_.u.t_string.ptr == nullptr) {
        RAISE(kRuntimeError, "could not allocate SValue");
      }

      memcpy(
          data_.u.t_string.ptr,
          copy.data_.u.t_string.ptr,
          data_.u.t_string.len);
      break;

    default:
      memcpy(&data_, &copy.data_, sizeof(data_));
      break;

  }

}

SValue& SValue::operator=(const SValue& copy) {
  if (data_.type == SQL_STRING) {
    free(data_.u.t_string.ptr);
  }

  if (copy.data_.type == SQL_STRING) {
    data_.type = SQL_STRING;
    data_.u.t_string.len = copy.data_.u.t_string.len;
    data_.u.t_string.ptr = static_cast<char *>(malloc(data_.u.t_string.len));

    if (data_.u.t_string.ptr == nullptr) {
      RAISE(kRuntimeError, "could not allocate SValue");
    }

    memcpy(
        data_.u.t_string.ptr,
        copy.data_.u.t_string.ptr,
        data_.u.t_string.len);
  } else {
    memcpy(&data_, &copy.data_, sizeof(data_));
  }

  return *this;
}

bool SValue::operator==(const SValue& other) const {
  switch (data_.type) {

    case SQL_INTEGER: {
      return getInteger() == other.getInteger();
    }

    case SQL_TIMESTAMP: {
      return getInteger() == other.getInteger();
    }

    case SQL_FLOAT: {
      return getFloat() == other.getFloat();
    }

    case SQL_BOOL: {
      return getBool() == other.getBool();
    }

    case SQL_STRING: {
      return memcmp(
          data_.u.t_string.ptr,
          other.data_.u.t_string.ptr,
          data_.u.t_string.len) == 0;
    }

    case SQL_NULL: {
      return other.getInteger() == 0;
    }

  }
}

sql_type SValue::getType() const {
  return data_.type;
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
  return getString();
}

template <> SValue::TimeType SValue::getValue<SValue::TimeType>() const {
  return getTimestamp();
}

// FIXPAUL: smarter type detection
template <> bool SValue::isConvertibleTo<SValue::BoolType>() const {
  return data_.type == SQL_BOOL;
}

template <> bool SValue::isConvertibleTo<SValue::TimeType>() const {
  if (data_.type == SQL_TIMESTAMP) {
    return true;
  }

  return isConvertibleToNumeric();
}

SValue::IntegerType SValue::getInteger() const {
  switch (data_.type) {

    case SQL_INTEGER:
      return data_.u.t_integer;

    case SQL_TIMESTAMP:
      return data_.u.t_timestamp;

    case SQL_FLOAT:
      return data_.u.t_float;

    case SQL_BOOL:
      return data_.u.t_bool;

    case SQL_NULL:
      return 0;

    case SQL_STRING:
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
          getString().c_str());

  }

  return 0;
}

SValue SValue::toInteger() const {
  if (data_.type == SQL_INTEGER) {
    return *this;
  } else {
    return SValue::newInteger(getInteger());
  }
}

SValue::FloatType SValue::getFloat() const {
  switch (data_.type) {

    case SQL_INTEGER:
      return data_.u.t_integer;

    case SQL_TIMESTAMP:
      return data_.u.t_timestamp;

    case SQL_FLOAT:
      return data_.u.t_float;

    case SQL_BOOL:
      return data_.u.t_bool;

    case SQL_NULL:
      return 0;

    case SQL_STRING:
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
          getString().c_str());

  }

  return 0;
}

SValue SValue::toFloat() const {
  if (data_.type == SQL_FLOAT) {
    return *this;
  } else {
    return SValue::newFloat(getFloat());
  }
}

SValue SValue::toBool() const {
  if (data_.type == SQL_BOOL) {
    return *this;
  } else {
    return SValue::newBool(getBool());
  }
}

SValue::BoolType SValue::getBool() const {
  switch (data_.type) {

    case SQL_INTEGER:
      return getInteger() > 0;

    case SQL_FLOAT:
      return getFloat() > 0;

    case SQL_BOOL:
      return data_.u.t_bool;

    case SQL_STRING:
      return true;

    case SQL_NULL:
      return false;

    default:
      RAISEF(
         kTypeError,
          "can't convert $0 '$1' to Boolean",
          SValue::getTypeName(data_.type),
          getString());

  }
}

SValue::TimeType SValue::getTimestamp() const {
  if (isTimestamp()) {
    return data_.u.t_timestamp;
  }

  if (isConvertibleTo<TimeType>()) {
    return toTimestamp().getTimestamp();
  } else {
    RAISE(
       kTypeError,
        "can't convert %s '%s' to TIMESTAMP",
        SValue::getTypeName(data_.type),
        getString().c_str());
  }
}

std::string SValue::makeUniqueKey(SValue* arr, size_t len) {
  std::string key;

  for (int i = 0; i < len; ++i) {
    key.append(arr[i].getString());
    key.append("\x00");
  }

  return key;
}

SValue SValue::toString() const {
  if (data_.type == SQL_STRING) {
    return *this;
  } else {
    return SValue::newString(getString());
  }
}


std::string SValue::getString() const {
  if (data_.type == SQL_STRING) {
    return std::string(data_.u.t_string.ptr, data_.u.t_string.len);
  }

  char buf[512];
  const char* str;
  size_t len;

  switch (data_.type) {

    case SQL_INTEGER: {
      len = snprintf(buf, sizeof(buf), "%" PRId64, getInteger());
      str = buf;
      break;
    }

    case SQL_TIMESTAMP: {
      return getTimestamp().toString("%Y-%m-%d %H:%M:%S");
    }

    case SQL_FLOAT: {
      len = snprintf(buf, sizeof(buf), "%f", getFloat());
      str = buf;
      break;
    }

    case SQL_BOOL: {
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

    case SQL_STRING: {
      return getString();
    }

    case SQL_NULL: {
      static const char undef_str[] = "NULL";
      str = undef_str;
      len = sizeof(undef_str) - 1;
    }

  }

  return std::string(str, len);
}

String SValue::toSQL() const {
  switch (data_.type) {

    case SQL_INTEGER: {
      return getString();
    }

    case SQL_TIMESTAMP: {
      return StringUtil::toString(getInteger());
    }

    case SQL_FLOAT: {
      return getString();
    }

    case SQL_BOOL: {
      return getString();
    }

    case SQL_STRING: {
      auto str = sql_escape(getString());
      return StringUtil::format("\"$0\"", str);
    }

    case SQL_NULL: {
      return "NULL";
    }

  }
}

const char* SValue::getTypeName(sql_type type) {
  switch (type) {
    case SQL_STRING:
      return "String";
    case SQL_FLOAT:
      return "Float";
    case SQL_INTEGER:
      return "Integer";
    case SQL_BOOL:
      return "Boolean";
    case SQL_TIMESTAMP:
      return "Timestamp";
    case SQL_NULL:
      return "NULL";
  }
}

const char* SValue::getTypeName() const {
  return SValue::getTypeName(data_.type);
}

template <> bool SValue::isConvertibleTo<SValue::IntegerType>() const {
  switch (data_.type) {
    case SQL_INTEGER:
    case SQL_TIMESTAMP:
      return true;
    default:
      break;
  }

  auto str = getString();
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

template <> bool SValue::isConvertibleTo<SValue::FloatType>() const {
  switch (data_.type) {
    case SQL_FLOAT:
    case SQL_INTEGER:
    case SQL_TIMESTAMP:
      return true;
    default:
      break;
  }

  auto str = getString();
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

template <> bool SValue::isConvertibleTo<std::string>() const {
  return true;
}

SValue SValue::toTimestamp() const {
  if (isTimestamp()) {
    return *this;
  }

  if (isConvertibleToNumeric()) {
    return SValue(SValue::TimeType(toNumeric().getFloat()));
  }

  RAISE(
      kTypeError,
      "can't convert %s '%s' to TIMESTAMP",
      SValue::getTypeName(data_.type),
      getString().c_str());
}

SValue SValue::toNumeric() const {
  if (isNumeric()) {
    return *this;
  }

  if (isConvertibleTo<SValue::IntegerType>()) {
    return SValue(SValue::IntegerType(getInteger()));
  }

  if (isConvertibleTo<SValue::FloatType>()) {
    return SValue(SValue::FloatType(getFloat()));
  }

  RAISE(
      kTypeError,
      "can't convert %s '%s' to NUMERIC",
      SValue::getTypeName(data_.type),
      getString().c_str());
}

bool SValue::isString() const {
  return data_.type == SQL_STRING;
}

bool SValue::isFloat() const {
  return data_.type == SQL_FLOAT;
}

bool SValue::isInteger() const {
  return data_.type == SQL_INTEGER;
}

bool SValue::isBool() const {
  return data_.type == SQL_BOOL;
}

bool SValue::isTimestamp() const {
  return data_.type == SQL_TIMESTAMP;
}

bool SValue::isNumeric() const {
  switch (data_.type) {
    case SQL_FLOAT:
    case SQL_INTEGER:
      return true;
    default:
      return false;
  }
}

bool SValue::isConvertibleToNumeric() const {
  if (isConvertibleTo<SValue::IntegerType>() ||
      isConvertibleTo<SValue::FloatType>() ||
      isTimestamp()) {
    return true;
  } else {
    return false;
  }
}

bool SValue::isConvertibleToBool() const {
  switch (data_.type) {
    case SQL_STRING:
    case SQL_FLOAT:
    case SQL_INTEGER:
    case SQL_BOOL:
    case SQL_NULL:
      return true;
    case SQL_TIMESTAMP:
      return false;
  }
}

void SValue::encode(OutputStream* os) const {
  os->appendUInt8(data_.type);

  switch (data_.type) {
    case SQL_STRING:
      os->appendLenencString(data_.u.t_string.ptr, data_.u.t_string.len);
      return;
    case SQL_FLOAT:
      os->appendDouble(data_.u.t_float);
      return;
    case SQL_INTEGER:
      os->appendUInt64(data_.u.t_integer);
      return;
    case SQL_BOOL:
      os->appendUInt8(data_.u.t_bool ? 1 : 0);
      return;
    case SQL_TIMESTAMP:
      os->appendUInt64(data_.u.t_timestamp);
      return;
    case SQL_NULL:
      return;
  }
}

void SValue::decode(InputStream* is) {
  auto type = is->readUInt8();

  switch (type) {
    case SQL_STRING:
      *this = SValue(is->readLenencString());
      return;
    case SQL_FLOAT:
      *this = SValue(SValue::FloatType(is->readDouble()));
      return;
    case SQL_INTEGER:
      *this = SValue(SValue::IntegerType(is->readUInt64()));
      return;
    case SQL_BOOL:
      *this = SValue(SValue::BoolType(is->readUInt8() == 1));
      return;
    case SQL_TIMESTAMP: {
      auto v = is->readUInt64();
      *this = SValue(SValue::TimeType(v));
      return;
    }
    case SQL_NULL:
      *this = SValue();
      return;
  }
}

String sql_escape(const String& orig_str) {
  auto str = orig_str;
  StringUtil::replaceAll(&str, "\\", "\\\\");
  StringUtil::replaceAll(&str, "'", "\\'");
  StringUtil::replaceAll(&str, "\"", "\\\"");
  return str;
}

template <> bool SValue::isOfType<SValue::StringType>() const {
  return isString();
}

template <> bool SValue::isOfType<SValue::FloatType>() const {
  return isFloat();
}

template <> bool SValue::isOfType<SValue::IntegerType>() const {
  return isInteger();
}

template <> bool SValue::isOfType<SValue::BoolType>() const {
  return isBool();
}

template <> bool SValue::isOfType<SValue::TimeType>() const {
  return isTimestamp();
}

}

namespace stx {

template <>
std::string inspect<sql_type>(
    const sql_type& type) {
  return csql::SValue::getTypeName(type);
}

template <>
std::string inspect<csql::SValue>(
    const csql::SValue& sval) {
  return sval.getString();
}

}

namespace std {

size_t hash<csql::SValue>::operator()(const csql::SValue& sval) const {
  return hash<std::string>()(sval.getString()); // FIXPAUL
}

}
