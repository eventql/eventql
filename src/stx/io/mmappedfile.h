/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_IO_MMAPPED_FILE_H_
#define _STX_IO_MMAPPED_FILE_H_
#include <stx/exception.h>
#include <stx/io/file.h>
#include <stx/VFS.h>
#include <stx/autoref.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>

namespace stx {
namespace io {

class MmappedFile : public VFSFile {
public:
  MmappedFile() = delete;
  MmappedFile(File&& file);
  MmappedFile(File&& file, size_t offset, size_t size);
  MmappedFile(const MmappedFile& copy) = delete;
  MmappedFile& operator=(const MmappedFile& copy) = delete;
  ~MmappedFile();

  void* data() const {
    return data_;
  }

  inline void* begin() const {
    return data_;
  }

  inline void* end() const {
    return ((char *) data_) + size_;
  }

  inline void* ptr() const {
    return data_;
  }

  size_t size() const {
    return size_;
  }

  bool isWritable() const;

protected:
  bool is_writable_;
  void* data_;
  void* mmap_;
  size_t size_;
  size_t mmap_size_;
};

}
}
#endif
