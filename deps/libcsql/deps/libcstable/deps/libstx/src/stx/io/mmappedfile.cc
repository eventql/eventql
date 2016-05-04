/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <sys/mman.h>
#include <unistd.h>
#include <stx/io/mmappedfile.h>

namespace stx {
namespace io {

MmappedFile::MmappedFile(File&& file) : MmappedFile(std::move(file), 0, -1) {}

MmappedFile::MmappedFile(File&& file, size_t offset, size_t size) {
  File local_file = std::move(file);
  is_writable_ = local_file.isWritable();
  mmap_size_ = local_file.size();

  if (mmap_size_ == 0) {
    RAISE(kIllegalArgumentError, "can't mmap() empty file");
  }

  mmap_ = mmap(
      nullptr,
      mmap_size_,
      is_writable_ ? PROT_WRITE | PROT_READ : PROT_READ,
      MAP_SHARED,
      local_file.fd(),
      0);

  if (mmap_ == MAP_FAILED) {
    RAISE_ERRNO(kIOError, "mmap() failed");
  }

  size_ = size == -1 ? mmap_size_ - offset : size;
  data_ = (char*) mmap_ + offset;
}

MmappedFile::~MmappedFile() {
  munmap(mmap_, mmap_size_);
}

bool MmappedFile::isWritable() const {
  return is_writable_;
}

}
}
