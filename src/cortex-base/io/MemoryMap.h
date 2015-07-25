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
#include <cortex-base/Buffer.h>

namespace cortex {

class CORTEX_API MemoryMap : public FixedBuffer {
 public:
  MemoryMap(int fd, off_t ofs, size_t size, bool rw);
  MemoryMap(MemoryMap&& mm);
  MemoryMap(const MemoryMap& mm) = delete;
  MemoryMap& operator=(MemoryMap&& mm);
  MemoryMap& operator=(MemoryMap& mm) = delete;
  ~MemoryMap();

  bool isReadable() const;
  bool isWritable() const;

  template<typename T> T* structAt(size_t ofs) const;

 private:
  int mode_;
};

template<typename T> inline T* MemoryMap::structAt(size_t ofs) const {
  return (T*) (((char*) data_) + ofs);
}

}  // namespace cortex
