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
#ifndef _STX_IO_MMAPPED_FILE_H_
#define _STX_IO_MMAPPED_FILE_H_
#include <eventql/util/exception.h>
#include <eventql/util/io/file.h>
#include <eventql/util/VFS.h>
#include <eventql/util/autoref.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>

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

#endif
