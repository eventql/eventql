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
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/exception.h>
#include <eventql/util/ieee754.h>

namespace util {

BinaryMessageWriter::BinaryMessageWriter(
    size_t initial_size) :
    ptr_(malloc(initial_size)),
    size_(initial_size),
    used_(0),
    owned_(true) {
  if (ptr_ == nullptr) {
    RAISE(kMallocError, "malloc() failed");
  }
}

BinaryMessageWriter::BinaryMessageWriter(
    void* buf,
    size_t buf_len) :
    ptr_(buf),
    size_(buf_len),
    used_(0),
    owned_(false) {}

BinaryMessageWriter::~BinaryMessageWriter() {
  if (owned_) {
    free(ptr_);
  }
}

void BinaryMessageWriter::appendUInt8(uint8_t value) {
  append(&value, sizeof(value));
}

void BinaryMessageWriter::appendUInt16(uint16_t value) {
  append(&value, sizeof(value));
}

void BinaryMessageWriter::updateUInt16(size_t offset, uint16_t value) {
  update(offset, &value, sizeof(value));
}

void BinaryMessageWriter::appendUInt32(uint32_t value) {
  append(&value, sizeof(value));
}

void BinaryMessageWriter::appendNUInt16(uint16_t value) {
  uint16_t v = htons(value);
  append(&v, sizeof(v));
}

void BinaryMessageWriter::updateNUInt16(size_t offset, uint16_t value) {
  uint16_t v = htons(value);
  update(offset, &v, sizeof(v));
}

void BinaryMessageWriter::updateUInt32(size_t offset, uint32_t value) {
  update(offset, &value, sizeof(value));
}

void BinaryMessageWriter::appendNUInt32(uint32_t value) {
  uint32_t v = htonl(value);
  append(&v, sizeof(v));
}

void BinaryMessageWriter::updateNUInt32(size_t offset, uint32_t value) {
  uint32_t v = htonl(value);
  update(offset, &v, sizeof(v));
}

void BinaryMessageWriter::appendUInt64(uint64_t value) {
  append(&value, sizeof(value));
}

void BinaryMessageWriter::updateUInt64(size_t offset, uint64_t value) {
  update(offset, &value, sizeof(value));
}

void BinaryMessageWriter::appendDouble(double value) {
  auto bytes = IEEE754::toBytes(value);
  append(&bytes, sizeof(bytes));
}

void BinaryMessageWriter::appendString(const std::string& string) {
  append(string.data(), string.size());
}

void BinaryMessageWriter::updateString(
    size_t offset,
    const std::string& string) {
  update(offset, string.data(), string.size());
}

void BinaryMessageWriter::appendLenencString(const std::string& string) {
  appendVarUInt(string.size());
  append(string.data(), string.size());
}

void BinaryMessageWriter::appendVarUInt(uint64_t value) {
  unsigned char buf[10];
  size_t bytes = 0;
  do {
    buf[bytes] = value & 0x7fU;
    if (value >>= 7) buf[bytes] |= 0x80U;
    ++bytes;
  } while (value);

  append(buf, bytes);
}


void* BinaryMessageWriter::data() const {
  return ptr_;
}

size_t BinaryMessageWriter::size() const {
  return used_;
}

void BinaryMessageWriter::append(void const* data, size_t size) {
  size_t resize = size_;

  while (used_ + size > resize) {
    resize *= 2;
  }

  if (resize > size_) {
    if (!owned_) {
      RAISE(kBufferOverflowError, "provided buffer is too small");
    }

    auto new_ptr = realloc(ptr_, resize);

    if (ptr_ == nullptr) {
      RAISE(kMallocError, "realloc() failed");
    }

    ptr_ = new_ptr;
  }

  memcpy(((char*) ptr_) + used_, data, size);
  used_ += size;
}

void BinaryMessageWriter::update(size_t offset, void const* data, size_t size) {
  if (offset + size > size_) {
    RAISE(kBufferOverflowError, "update exceeds buffer boundary");
  }

  memcpy(((char*) ptr_) + offset, data, size);
}

void BinaryMessageWriter::clear() {
  used_ = 0;
}

template <>
void BinaryMessageWriter::appendValue<uint16_t>(const uint16_t& val) {
  appendUInt16(val);
}

template <>
void BinaryMessageWriter::appendValue<uint32_t>(const uint32_t& val) {
  appendUInt32(val);
}

template <>
void BinaryMessageWriter::appendValue<String>(const String& val) {
  appendUInt32(val.size());
  append(val.data(), val.size());
}

}

