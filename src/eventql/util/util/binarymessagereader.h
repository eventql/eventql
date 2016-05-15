/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#ifndef _STX_UTIL_BINARYMESSAGEREADER_H
#define _STX_UTIL_BINARYMESSAGEREADER_H
#include <stdlib.h>
#include <stdint.h>
#include <eventql/util/exception.h>
#include <eventql/util/ieee754.h>
#include <string>

namespace stx {
namespace util {

class BinaryMessageReader {
public:
  BinaryMessageReader(void const* buf, size_t buf_len);
  virtual ~BinaryMessageReader() {};

  inline uint8_t const* readUInt8();
  inline uint16_t const* readUInt16();
  inline uint32_t const* readUInt32();
  inline uint64_t const* readUInt64();
  inline uint64_t readVarUInt();
  inline char const* readString(size_t size);
  inline void const* read(size_t size);
  inline std::string readLenencString();
  inline double readDouble();

  inline bool maybeReadUInt8(uint8_t* val);
  inline bool maybeReadUInt16(uint16_t* val);
  inline bool maybeReadUInt32(uint32_t* val);
  inline bool maybeReadUInt64(uint64_t* val);
  inline bool maybeReadVarUInt(uint64_t* val);
  inline bool maybeReadLenencString(std::string* val);
  inline bool maybeReadDouble(double* val);

  template <typename T>
  inline T const* readValue();

  inline void rewind();
  inline void seekTo(size_t pos);

  inline size_t remaining() const;
  inline size_t position() const;

protected:
  void const* ptr_;
  size_t size_;
  size_t pos_;
  std::string cur_str_;
};

}
}

#include "binarymessagereader_impl.h"
#endif
