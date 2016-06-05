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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/exception.h>
#include <eventql/util/autoref.h>
#include <eventql/util/buffer.h>
#include <eventql/util/io/file.h>
#include <eventql/io/cstable/cstable.h>

namespace cstable {

struct PageRef {
  uint64_t offset;
  uint32_t size;
};

struct PageIndexKey {
  uint32_t column_id;
  PageIndexEntryType entry_type;
};

class PageManager : public RefCounted {
public:

  PageManager(
      BinaryFormatVersion version,
      File&& file,
      uint64_t offset);

  PageRef allocPage(
      PageIndexKey key,
      uint32_t size);

  void writeToPage(
      const PageRef& page,
      size_t pos,
      const void* data,
      size_t len);

  void flushPage(const PageRef& page);

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

