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
#include <string>

namespace stx {
namespace util {

class BinaryMessageReader {
public:
  BinaryMessageReader(void const* buf, size_t buf_len);
  virtual ~BinaryMessageReader() {};

  uint8_t const* readUInt8();
  uint16_t const* readUInt16();
  uint32_t const* readUInt32();
  uint64_t const* readUInt64();
  uint64_t readVarUInt();
  char const* readString(size_t size);
  void const* read(size_t size);
  std::string readLenencString();
  double readDouble();

  template <typename T>
  T const* readValue();

  void rewind();
  void seekTo(size_t pos);

  size_t remaining() const;
  size_t position() const;

protected:
  void const* ptr_;
  size_t size_;
  size_t pos_;
  std::string cur_str_;
};

}
}

#endif
