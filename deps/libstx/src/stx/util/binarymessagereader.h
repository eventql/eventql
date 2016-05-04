/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_UTIL_BINARYMESSAGEREADER_H
#define _STX_UTIL_BINARYMESSAGEREADER_H
#include <stdlib.h>
#include <stdint.h>
#include <stx/exception.h>
#include <stx/ieee754.h>
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
