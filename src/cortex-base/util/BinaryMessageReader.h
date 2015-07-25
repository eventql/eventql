// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/Api.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>

namespace cortex {
namespace util {

class CORTEX_API BinaryMessageReader {
public:
  BinaryMessageReader(void const* buf, size_t buf_len);
  virtual ~BinaryMessageReader() {};

  uint16_t const* readUInt16();
  uint32_t const* readUInt32();
  uint64_t const* readUInt64();
  char const* readString(size_t size);
  void const* read(size_t size);

  void rewind();
  void seekTo(size_t pos);

  size_t remaining() const;

protected:
  void const* ptr_;
  size_t size_;
  size_t pos_;
};

} // namespace util
} // namespace cortex
