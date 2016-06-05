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
#include <eventql/io/cstable/io/PageManager.h>
#include <eventql/io/cstable/cstable.h>
#include <eventql/util/exception.h>
#include <eventql/util/freeondestroy.h>


namespace cstable {

PageManager::PageManager(
    BinaryFormatVersion version,
    File&& file,
    uint64_t offset) :
    version_(version),
    file_(std::move(file)),
    offset_(offset) {
  switch (version) {
    case BinaryFormatVersion::v0_1_0:
      meta_block_position_ = 0,
      meta_block_size_ = 0;
    case BinaryFormatVersion::v0_2_0:
      meta_block_position_ = cstable::v0_2_0::kMetaBlockPosition,
      meta_block_size_ = cstable::v0_2_0::kMetaBlockSize;
      break;
  }
}

PageRef PageManager::allocPage(PageIndexKey key, uint32_t size) {
  PageRef page;
  page.offset = offset_;
  page.size = padToNextSector(size);
  offset_ += page.size;
  return page;
}

//void PageManager::writePage(const PageRef& page, const Buffer& buffer) {
//  writePage(page.offset, page.size, buffer.data(), buffer.size());
//}
//
//void PageManager::writePage(
//    uint64_t page_offset,
//    uint64_t page_size,
//    const void* data,
//    size_t data_size) {
//  auto page_data = data;
//
//  FreeOnDestroy tmp;
//  if (data_size < page_size) {
//    page_data = malloc(page_size);
//    if (!page_data) {
//      RAISE(kMallocError, "malloc() failed");
//    }
//
//    tmp.store((void*) page_data);
//    memcpy(tmp.get(), data, data_size);
//    memset((char*) tmp.get() + data_size, 0, page_size - data_size);
//  }
//
//  file_.pwrite(page_offset, page_data, page_size);
//}

void PageManager::writeTransaction(const MetaBlock& mb) {
  // fsync all changes before writing new tx
  file_.fsync();

  // build new meta block
  Buffer buf;
  buf.reserve(meta_block_size_);
  auto os = BufferOutputStream::fromBuffer(&buf);

  switch (version_) {
    case BinaryFormatVersion::v0_1_0:
      RAISE(kIllegalArgumentError, "unsupported version: v0.1.0");
    case BinaryFormatVersion::v0_2_0:
      cstable::v0_2_0::writeMetaBlock(mb, os.get());
      break;
  }

  RCHECK(buf.size() == meta_block_size_, "invalid meta block size");

  // write to metablock slot
  auto mb_index = mb.transaction_id % 2;
  auto mb_offset = meta_block_position_ + meta_block_size_ * mb_index;
  file_.pwrite(mb_offset, buf.data(), buf.size());

  // fsync one last time
  file_.fsync();
}

uint64_t PageManager::getOffset() const {
  return offset_;
}

File* PageManager::file() {
  return &file_;
}

} // namespace cstable


