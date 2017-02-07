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
#define __STDC_FORMAT_MACROS
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string>
#include <ctime>
#include <stdint.h>
#include <iostream>
#include <eventql/util/inspect.h>
#include <eventql/util/human.h>
#include <eventql/sql/svalue.h>
#include <eventql/sql/format.h>
#include <eventql/sql/parser/token.h>

namespace csql {


SValue SValue::newNull() {
  return SValue();
}

SValue SValue::newUInt64(uint64_t value) {
  static_assert(
      sizeof(uint64_t) + sizeof(STag) <= SValue::kInlineDataSize,
      "SValue::kInlineDataSize is too small");

  SValue v(SType::UINT64, DEFER_INITIALIZATION);
  STag tag_inline = STAG_INLINE;
  memcpy(v.data_, &value, sizeof(uint64_t));
  memset(v.data_ + sizeof(uint64_t), 0, sizeof(STag));
  memcpy(v.data_ + (sizeof(data_) - sizeof(STag)), &tag_inline, sizeof(STag));
  return v;
}

SValue SValue::newUInt64(const std::string& value) {
  return SValue::newUInt64(std::stoull(value));
}

SValue SValue::newInt64(int64_t value) {
  static_assert(
      sizeof(int64_t) + sizeof(STag) <= SValue::kInlineDataSize,
      "SValue::kInlineDataSize is too small");

  SValue v(SType::INT64, DEFER_INITIALIZATION);
  STag tag_inline = STAG_INLINE;
  memcpy(v.data_, &value, sizeof(int64_t));
  memset(v.data_ + sizeof(int64_t), 0, sizeof(STag));
  memcpy(v.data_ + (sizeof(data_) - sizeof(STag)), &tag_inline, sizeof(STag));
  return v;
}

SValue SValue::newInt64(const std::string& value) {
  return SValue::newInt64(std::stoll(value));
}

SValue SValue::newFloat64(double value) {
  static_assert(
      sizeof(double) + sizeof(STag) <= SValue::kInlineDataSize,
      "SValue::kInlineDataSize is too small");

  SValue v(SType::FLOAT64, DEFER_INITIALIZATION);
  STag tag_inline = STAG_INLINE;
  memcpy(v.data_, &value, sizeof(double));
  memset(v.data_ + sizeof(double), 0, sizeof(STag));
  memcpy(v.data_ + (sizeof(data_) - sizeof(STag)), &tag_inline, sizeof(STag));
  return v;
}

SValue SValue::newFloat64(const std::string& value) {
  return SValue::newFloat64(std::stod(value));
}

SValue SValue::newBool(bool value) {
  static_assert(
      sizeof(uint8_t) + sizeof(STag) <= SValue::kInlineDataSize,
      "SValue::kInlineDataSize is too small");

  uint8_t value_uint = value;
  SValue v(SType::BOOL, DEFER_INITIALIZATION);
  STag tag_inline = STAG_INLINE;
  memcpy(v.data_, &value_uint, sizeof(uint8_t));
  memset(v.data_ + sizeof(uint8_t), 0, sizeof(STag));
  memcpy(v.data_ + (sizeof(data_) - sizeof(STag)), &tag_inline, sizeof(STag));
  return v;
}

SValue SValue::newString(const char* value, uint32_t value_len) {
  static_assert(
      sizeof(intptr_t) + sizeof(uint32_t) + sizeof(STag) <= SValue::kInlineDataSize,
      "SValue::kInlineDataSize is too small");

  SValue v(SType::STRING, DEFER_INITIALIZATION);
  if (value_len + sizeof(uint32_t) + sizeof(STag) <= kInlineDataSize) {
    STag tag = STAG_INLINE;
    memcpy(v.data_, &value_len, sizeof(uint32_t));
    memcpy(v.data_ + sizeof(uint32_t), value, value_len);
    memset(v.data_ + sizeof(uint32_t) + value_len, 0, sizeof(STag));
    memcpy(v.data_ + (sizeof(data_) - sizeof(STag)), &tag, sizeof(STag));
  } else {
    STag tag = 0;
    uint32_t ptr_capacity = sizeof(uint32_t) + value_len + sizeof(STag);
    intptr_t ptr = reinterpret_cast<intptr_t>(malloc(ptr_capacity));
    memcpy(v.data_, &ptr, sizeof(intptr_t));
    memcpy(v.data_ + sizeof(intptr_t), &ptr_capacity, sizeof(uint32_t));
    memcpy(v.data_ + (sizeof(data_) - sizeof(STag)), &tag, sizeof(STag));
    memcpy(reinterpret_cast<void*>(ptr), &value_len, sizeof(uint32_t));
    memcpy(reinterpret_cast<void*>(ptr + sizeof(uint32_t)), value, value_len);
    memset(reinterpret_cast<void*>(ptr + sizeof(uint32_t) + value_len), 0, sizeof(SType));
  }

  return v;
}

SValue SValue::newString(const std::string& str) {
  return newString(str.data(), str.size());
}

SValue SValue::newTimestamp64(uint64_t value) {
  static_assert(
      sizeof(uint64_t) + sizeof(STag) <= SValue::kInlineDataSize,
      "SValue::kInlineDataSize is too small");

  SValue v(SType::UINT64);
  STag tag_inline = STAG_INLINE;
  memcpy(v.data_, &value, sizeof(uint64_t));
  memset(v.data_ + sizeof(uint64_t), 0, sizeof(STag));
  memcpy(v.data_ + (sizeof(data_) - sizeof(STag)), &tag_inline, sizeof(STag));
  return v;
}


SValue::SValue() : SValue(SType::NIL) {}

SValue::SValue(SType type) : type_(type) {
  STag tag_inline = STAG_INLINE;
  memset(data_, 0, sizeof(data_) - sizeof(STag));
  memcpy(data_ + (sizeof(data_) - sizeof(STag)), &tag_inline, sizeof(STag));
}

SValue::SValue(SType type, kDeferInitialization _) : type_(type) {}

SValue::SValue(const SValue& copy) : type_(copy.type_) {
  memcpy(data_, copy.data_, sizeof(data_));

  STag tag;
  memcpy(&tag, data_ + (sizeof(data_) - sizeof(STag)), sizeof(STag));
  if (!(tag & STAG_INLINE)) {
    intptr_t ptr;
    memcpy(&ptr, data_, sizeof(intptr_t));
    uint32_t ptr_capacity;
    memcpy(&ptr_capacity, data_ + sizeof(intptr_t), sizeof(uint32_t));

    intptr_t new_ptr = reinterpret_cast<intptr_t>(malloc(ptr_capacity));
    memcpy(
        reinterpret_cast<void*>(new_ptr),
        reinterpret_cast<void*>(ptr),
        ptr_capacity);

    memcpy(data_, &new_ptr, sizeof(intptr_t));
  }
}

SValue::~SValue() {
  STag tag;
  memcpy(&tag, data_ + (sizeof(data_) - sizeof(STag)), sizeof(STag));
  if (!(tag & STAG_INLINE)) {
    intptr_t ptr;
    memcpy(&ptr, data_, sizeof(intptr_t));
    free(reinterpret_cast<void*>(ptr));
  }
}

SValue& SValue::operator=(const SValue& copy) {
  type_ = copy.type_;

  STag tag;
  memcpy(&tag, data_ + (sizeof(data_) - sizeof(STag)), sizeof(STag));
  if (!(tag & STAG_INLINE)) {
    intptr_t ptr;
    memcpy(&ptr, data_, sizeof(intptr_t));
    free(reinterpret_cast<void*>(ptr));
  }

  memcpy(data_, copy.data_, sizeof(data_));
  memcpy(&tag, data_ + (sizeof(data_) - sizeof(STag)), sizeof(STag));
  if (!(tag & STAG_INLINE)) {
    intptr_t ptr;
    memcpy(&ptr, data_, sizeof(intptr_t));
    uint32_t ptr_capacity;
    memcpy(&ptr_capacity, data_ + sizeof(intptr_t), sizeof(uint32_t));

    intptr_t new_ptr = reinterpret_cast<intptr_t>(malloc(ptr_capacity));
    memcpy(
        reinterpret_cast<void*>(new_ptr),
        reinterpret_cast<void*>(ptr),
        ptr_capacity);

    memcpy(data_, &new_ptr, sizeof(intptr_t));
  }

  return *this;
}

bool SValue::operator==(const SValue& other) const {
  if (type_ != other.type_) {
    return false;
  }

  auto size = getSize();
  if (size != other.getSize()) {
    return false;
  }

  return memcmp(getData(), other.getData(), size) == 0;
}

SType SValue::getType() const {
  return type_;
}

bool SValue::isUInt64() const {
  return type_ == SType::UINT64;
}

bool SValue::isInt64() const {
  return type_ == SType::INT64;
}

bool SValue::isFloat64() const {
  return type_ == SType::FLOAT64;
}

bool SValue::isBool() const {
  return type_ == SType::BOOL;
}

bool SValue::isString() const {
  return type_ == SType::STRING;
}

bool SValue::isTimestamp64() const {
  return type_ == SType::TIMESTAMP64;
}

std::string SValue::toString() const {
  return sql_tostring(type_, getData());
}

//String SValue::toSQL() const {
//  switch (data_.type) {
//
//    case SType::INT64:
//    case SType::UINT64: {
//      return getString();
//    }
//
//    case SType::TIMESTAMP64: {
//      return StringUtil::toString(getInteger());
//    }
//
//    case SType::FLOAT64: {
//      return getString();
//    }
//
//    case SType::BOOL: {
//      return getString();
//    }
//
//    case SType::STRING: {
//      auto str = sql_escape(getString());
//      return StringUtil::format("\"$0\"", str);
//    }
//
//    case SType::NIL: {
//      return "NULL";
//    }
//
//  }
//}

void SValue::encode(OutputStream* os) const {
  os->appendUInt8((uint8_t) type_);
  os->appendLenencString(getData(), getSize());
}

void SValue::decode(InputStream* is) {
  type_ = (SType) is->readUInt8();
  auto data = is->readLenencString();
  setData(data.data(), data.size());
}

void SValue::copyFrom(const void* data) {
  auto data_len = sql_sizeof(type_, data);
  setData(data, data_len);
}

const void* SValue::getData() const {
  STag tag;
  memcpy(&tag, data_ + (sizeof(data_) - sizeof(STag)), sizeof(STag));
  if (tag & STAG_INLINE) {
    return data_;
  } else {
    intptr_t ptr;
    memcpy(&ptr, data_, sizeof(intptr_t));
    return reinterpret_cast<void*>(ptr);
  }
}

void* SValue::getData() {
  STag tag;
  memcpy(&tag, data_ + (sizeof(data_) - sizeof(STag)), sizeof(STag));
  if (tag & STAG_INLINE) {
    return data_;
  } else {
    intptr_t ptr;
    memcpy(&ptr, data_, sizeof(intptr_t));
    return reinterpret_cast<void*>(ptr);
  }
}

void SValue::setData(const void* data, size_t size) {
  static_assert(
      sizeof(intptr_t) + sizeof(uint32_t) + sizeof(STag) <= SValue::kInlineDataSize,
      "SValue::kInlineDataSize is too small");

  bool needs_ext_mem = size > kInlineDataSize;
  bool has_ext_mem;
  {
    STag tag;
    memcpy(&tag, data_ + (sizeof(data_) - sizeof(STag)), sizeof(STag));
    has_ext_mem = !(tag & STAG_INLINE);
  }

  if (!has_ext_mem && !needs_ext_mem) {
    memcpy(data_, data, size);
    STag tag;
    memcpy(&tag, data_ + (sizeof(data_) - sizeof(STag)), sizeof(STag));
    tag |= STAG_INLINE;
    memcpy(data_ + (sizeof(data_) - sizeof(STag)), &tag, sizeof(STag));
    return;
  }

  intptr_t ptr;
  if (has_ext_mem) {
    uint32_t ptr_capacity;
    memcpy(&ptr, data_, sizeof(intptr_t));
    memcpy(&ptr_capacity, data_ + sizeof(intptr_t), sizeof(uint32_t));

    if (ptr_capacity < size) {
      uint32_t ptr_capacity = size;
      ptr = reinterpret_cast<intptr_t>(
          realloc(reinterpret_cast<void*>(ptr), ptr_capacity));

      memcpy(data_, &ptr, sizeof(intptr_t));
      memcpy(data_ + sizeof(intptr_t), &ptr_capacity, sizeof(uint32_t));
    }
  } else {
    uint32_t ptr_capacity = size;
    ptr = reinterpret_cast<intptr_t>(malloc(ptr_capacity));
    memcpy(data_, &ptr, sizeof(intptr_t));
    memcpy(data_ + sizeof(intptr_t), &ptr_capacity, sizeof(uint32_t));
    STag tag = 0;
    memcpy(data_ + (sizeof(data_) - sizeof(STag)), &tag, sizeof(STag));
  }

  memcpy(reinterpret_cast<void*>(ptr), data, size);
}

size_t SValue::getSize() const {
  return sql_sizeof(type_, getData());
}

void SValue::setTag(STag tag) {
  auto data = getData();
  memcpy(data + sql_sizeof(type_, data) - sizeof(STag), &tag, sizeof(STag));
}

STag SValue::getTag() const {
  STag tag;
  auto data = getData();
  memcpy(&tag, data + sql_sizeof(type_, data) - sizeof(STag), sizeof(STag));
  return tag;
}

String sql_escape(const String& orig_str) {
  auto str = orig_str;
  StringUtil::replaceAll(&str, "\\", "\\\\");
  StringUtil::replaceAll(&str, "'", "\\'");
  StringUtil::replaceAll(&str, "\"", "\\\"");
  return str;
}

SVector::SVector(
    SType type) :
    type_(type),
    data_(nullptr),
    capacity_(0),
    size_(0) {}

SVector::SVector(
    SVector&& other) :
    type_(other.type_),
    data_(other.data_),
    capacity_(other.capacity_),
    size_(other.size_) {
  other.data_ = nullptr;
  other.capacity_ = 0;
  other.size_ = 0;
}

SVector::~SVector() {
  if (data_) {
    free(data_);
  }
}

SType SVector::getType() const {
  return type_;
}

const void* SVector::getData() const {
  return data_;
}

void* SVector::getMutableData() {
  return data_;
}

size_t SVector::getSize() const {
  return size_;
}

void SVector::setSize(size_t new_size) {
  assert(new_size <= capacity_);
  size_ = new_size;
}

void SVector::clear() {
  size_ = 0;
}

size_t SVector::getCapacity() const {
  return capacity_;
}

void SVector::increaseCapacity(size_t min_capacity) {
  if (min_capacity <= capacity_) {
    return;
  }

  auto new_capactity = min_capacity; // FIXME
  if (data_) {
    data_ = realloc(data_, new_capactity);
  } else {
    data_ = malloc(new_capactity);
  }

  capacity_ = new_capactity;
}

void SVector::copyFrom(const SVector* other) {
  increaseCapacity(other->capacity_);
  size_ = other->size_;
  memcpy(data_, other->data_, size_);
}

void SVector::append(const void* data, size_t size) {
  if (size_ + size > capacity_) {
    increaseCapacity(size_ + size);
  }

  memcpy((char*) data_ + size_, data, size);
  size_ += size;
}

void SVector::append(const SValue& svalue) {
  append(svalue.getData(), svalue.getSize());
  auto tag = svalue.getTag();
  append(&tag, sizeof(STag));
}

size_t SVector::next(SType type, void** cursor) {
  auto elen = sql_sizeof(type, *cursor);
  *cursor = (char*) *cursor + elen;
  return elen;
}

size_t SVector::next(void** cursor) const {
  return next(type_, cursor);
}

size_t sql_strlen(const void* str) {
  return *((const uint32_t*) str);
}

char* sql_cstr(void* str) {
  return (char*) str + sizeof(uint32_t);
}

const char* sql_cstr(const void* str) {
  return (const char*) str + sizeof(uint32_t);
}

size_t sql_sizeof(SType type, const void* data) {
  switch (type) {
    case SType::STRING:
      return sizeof(uint32_t) + sql_strlen(data) + sizeof(STag);
    case SType::NIL:
      return 0 + sizeof(STag);
    case SType::FLOAT64:
    case SType::INT64:
    case SType::UINT64:
    case SType::TIMESTAMP64:
      return 8 + sizeof(STag);
    case SType::BOOL:
      return 1 + sizeof(STag);
  }
}

size_t sql_sizeof_static(SType type) {
  switch (type) {
    case SType::STRING:
      throw std::runtime_error("invalid call of sql_sizeof_static(STRING)");
    case SType::NIL:
      return 0 + sizeof(STag);
    case SType::FLOAT64:
    case SType::INT64:
    case SType::UINT64:
    case SType::TIMESTAMP64:
      return 8 + sizeof(STag);
    case SType::BOOL:
      return 1 + sizeof(STag);
  }
}

std::string sql_tostring(SType type, const void* value) {
  if (!value) {
    return "NULL";
  }

  switch (type) {

    case SType::NIL:
      return "NULL";

    case SType::INT64: {
      STag tag;
      memcpy(&tag, (const char*) value + sizeof(int64_t), sizeof(STag));
      if (tag & STAG_NULL) {
        return "NULL";
      } else {
        return std::to_string(*static_cast<const int64_t*>(value));
      }
    }

    case SType::UINT64: {
      STag tag;
      memcpy(&tag, (const char*) value + sizeof(uint64_t), sizeof(STag));
      if (tag & STAG_NULL) {
        return "NULL";
      } else {
        return std::to_string(*static_cast<const uint64_t*>(value));
      }
    }

    case SType::FLOAT64: {
      STag tag;
      memcpy(&tag, (const char*) value + sizeof(double), sizeof(STag));
      if (tag & STAG_NULL) {
        return "NULL";
      } else {
        return std::to_string(*static_cast<const double*>(value));
      }
    }

    case SType::STRING: {
      auto strlen = sql_strlen(value);
      STag tag;
      memcpy(&tag, (const char*) value + sizeof(uint32_t) + strlen, sizeof(STag));
      if (tag & STAG_NULL) {
        return "NULL";
      } else {
        return std::string(sql_cstr(value), strlen);
      }
    }

    case SType::TIMESTAMP64: {
      STag tag;
      memcpy(&tag, (const char*) value + sizeof(uint64_t), sizeof(STag));
      if (tag & STAG_NULL) {
        return "NULL";
      } else {
        return UnixTime(*static_cast<const uint64_t*>(value)).toString();
      }
    }

    case SType::BOOL: {
      STag tag;
      memcpy(&tag, (const char*) value + sizeof(uint8_t), sizeof(STag));
      if (tag & STAG_NULL) {
        return "NULL";
      } else {
        return *static_cast<const uint8_t*>(value) ? "true" : "false";
      }
    }

  }
}

} // namespace csql

