// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>

namespace cortex {
namespace util {

class CORTEX_API BinaryMessageWriter {
public:
  static const size_t kInitialDataSize = 4096;

  BinaryMessageWriter(size_t initial_size = kInitialDataSize);
  BinaryMessageWriter(void* buf, size_t buf_len);
  ~BinaryMessageWriter();

  void appendUInt16(uint16_t value);
  void appendUInt32(uint32_t value);
  void appendUInt64(uint64_t value);
  void appendString(const std::string& string);
  void append(void const* data, size_t size);

  void updateUInt16(size_t offset, uint16_t value);
  void updateUInt32(size_t offset, uint32_t value);
  void updateUInt64(size_t offset, uint64_t value);
  void updateString(size_t offset, const std::string& string);
  void update(size_t offset, void const* data, size_t size);

  void* data() const;
  size_t size() const;

protected:
  void* ptr_;
  size_t size_;
  size_t used_;
  bool owned_;
};

} // namespace util
} // namespace cortex
