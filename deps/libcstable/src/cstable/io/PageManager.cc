/**
 * This file is part of the "libcstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libcstable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/io/PageManager.h>
#include <cstable/cstable.h>
#include <stx/exception.h>
#include <stx/freeondestroy.h>


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

PageRef PageManager::allocPage(uint32_t size) {
  PageRef page;
  page.offset = offset_;
  page.size = padToNextSector(size);
  offset_ += page.size;
  return page;
}

void PageManager::writePage(const PageRef& page, const Buffer& buffer) {
  writePage(page.offset, page.size, buffer.data(), buffer.size());
}

void PageManager::writePage(
    uint64_t page_offset,
    uint64_t page_size,
    const void* data,
    size_t data_size) {
  auto page_data = data;

  stx::util::FreeOnDestroy tmp;
  if (data_size < page_size) {
    page_data = malloc(page_size);
    if (!page_data) {
      RAISE(kMallocError, "malloc() failed");
    }

    tmp.store((void*) page_data);
    memcpy(tmp.get(), data, data_size);
    memset((char*) tmp.get() + data_size, 0, page_size - data_size);
  }

  file_.pwrite(page_offset, page_data, page_size);
}

void PageManager::writeTransaction(const MetaBlock& mb) {
  RCHECK(
      version_ != BinaryFormatVersion::v0_1_0,
      "writeTransaction is not supported in v0.1.x");

  // fsync all changes before writing new tx
  file_.fsync();

  // build new meta block
  Buffer buf;
  buf.reserve(meta_block_size_);
  auto os = stx::BufferOutputStream::fromBuffer(&buf);

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


