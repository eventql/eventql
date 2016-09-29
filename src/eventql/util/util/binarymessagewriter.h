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
#ifndef _STX_UTIL_BINARYMESSAGEWRITER_H
#define _STX_UTIL_BINARYMESSAGEWRITER_H
#include <stdlib.h>
#include <stdint.h>
#include <string>

namespace util {

class BinaryMessageWriter {
public:
  static const size_t kInitialDataSize = 4096;

  BinaryMessageWriter(size_t initial_size = kInitialDataSize);
  BinaryMessageWriter(void* buf, size_t buf_len);
  ~BinaryMessageWriter();

  void appendUInt8(uint8_t value);
  void appendUInt16(uint16_t value);
  void appendUInt32(uint32_t value);
  void appendUInt64(uint64_t value);
  void appendNUInt16(uint16_t value);
  void appendNUInt32(uint32_t value);
  void appendNUInt64(uint64_t value);
  void appendVarUInt(uint64_t value);
  void appendDouble(double value);
  void appendString(const std::string& string);
  void appendLenencString(const std::string& string);
  void append(void const* data, size_t size);

  template <typename T>
  void appendValue(const T& val);

  void updateUInt16(size_t offset, uint16_t value);
  void updateUInt32(size_t offset, uint32_t value);
  void updateUInt64(size_t offset, uint64_t value);
  void updateNUInt16(size_t offset, uint16_t value);
  void updateNUInt32(size_t offset, uint32_t value);
  void updateNUInt64(size_t offset, uint64_t value);
  void updateDouble(size_t offset, double value);
  void updateString(size_t offset, const std::string& string);
  void update(size_t offset, void const* data, size_t size);

  void* data() const;
  size_t size() const;

  void clear();

protected:
  void* ptr_;
  size_t size_;
  size_t used_;
  bool owned_;
};

}

#endif
