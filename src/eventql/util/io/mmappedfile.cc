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
#include <sys/mman.h>
#include <unistd.h>
#include <eventql/util/io/mmappedfile.h>

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
