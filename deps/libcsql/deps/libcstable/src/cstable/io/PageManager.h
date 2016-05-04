/**
 * This file is part of the "libcstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libcstable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/exception.h>
#include <stx/autoref.h>
#include <stx/buffer.h>
#include <stx/io/file.h>
#include <cstable/cstable.h>

namespace cstable {

struct PageRef {
  uint64_t offset;
  uint32_t size;
};

class PageManager : public stx::RefCounted {
public:

  PageManager(
      BinaryFormatVersion version,
      File&& file,
      uint64_t offset);

  PageRef allocPage(uint32_t size);

  void writePage(const PageRef& page, const Buffer& buffer);

  /**
   * data_size must be less than or equal to page size. if it is less than page
   * size an extra memory allocation will be performed
   */
  void writePage(
      uint64_t page_offset,
      uint64_t page_size,
      const void* data,
      size_t data_size);

  void writeTransaction(const MetaBlock& mb);

  uint64_t getOffset() const;

  File* file();

protected:
  BinaryFormatVersion version_;
  File file_;
  uint64_t offset_;
  size_t meta_block_position_;
  size_t meta_block_size_;
};

} // namespace cstable

