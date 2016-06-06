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
#include <eventql/io/cstable/page_manager.h>
#include <eventql/io/cstable/cstable.h>
#include <eventql/util/exception.h>
#include <eventql/util/freeondestroy.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

namespace cstable {

PageManager::PageManager(
    int fd,
    uint64_t offset,
    const Vector<PageIndexEntry>& index) :
    fd_(fd),
    allocated_bytes_(offset),
    index_(index) {}

PageManager::~PageManager() {
  flushAllPages();
}

PageRef PageManager::allocPage(PageIndexKey key, uint32_t size) {
  std::unique_lock<std::mutex> lk(mutex_);

  PageRef page;
  page.offset = allocated_bytes_;
  page.size = padToNextSector(size);

  PageIndexEntry idx_entry;
  idx_entry.key = key;
  idx_entry.page = page;

  auto buf = malloc(page.size);
  if (buf) {
    memset(buf, 0, page.size);
  } else {
    RAISE(kMallocError, "malloc() failed");
  }

  allocated_bytes_ += page.size;
  buffered_pages_.emplace(page.offset, buf);
  index_.emplace_back(idx_entry);
  return page;
}

void PageManager::writeToPage(
    const PageRef& page,
    size_t pos,
    const void* data,
    size_t len) {
  assert(pos + len <= page.size);
  std::unique_lock<std::mutex> lk(mutex_);

  auto buf = buffered_pages_.find(page.offset);
  if (buf == buffered_pages_.end()) {
    RAISE(kIllegalArgumentError, "invalid write access");
  }

  memcpy((char*) buf->second + pos, data, len);
}

void PageManager::flushPageWithLock(const PageRef& page) {
  if (fd_ < 0) {
    return;
  }

  auto buf = buffered_pages_.find(page.offset);
  if (buf == buffered_pages_.end()) {
    return;
  }

  auto ret = pwrite(fd_, buf->second, page.size, page.offset);
  if (ret < 0) {
    RAISE_ERRNO(kIOError, "write() failed");
  }
  if (ret != page.size) {
    RAISE(kIOError, "write() failed");
  }

  buffered_pages_.erase(buf);
}

void PageManager::flushPage(const PageRef& page) {
  std::unique_lock<std::mutex> lk(mutex_);
  flushPageWithLock(page);
}

void PageManager::flushAllPages() {
  std::unique_lock<std::mutex> lk(mutex_);
  for (const auto& p : index_) {
    flushPageWithLock(p.page);
  }
}

void PageManager::readPage(
    const PageRef& page,
    void* data) const {
  std::unique_lock<std::mutex> lk(mutex_);
  auto buf = buffered_pages_.find(page.offset);
  if (buf != buffered_pages_.end()) {
    memcpy(data, buf->second, page.size);
    return;
  }

  auto ret = pread(fd_, data, page.size, page.offset);
  if (ret < 0) {
    RAISE_ERRNO(kIOError, "read() failed");
  }
  if (ret != page.size) {
    RAISE(kIOError, "read() failed");
  }
}

uint64_t PageManager::getAllocatedBytes() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return allocated_bytes_;
}

int PageManager::getFD() const {
  return fd_;
}

Vector<PageIndexEntry> PageManager::getPageIndex() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return index_;
}

Vector<PageRef> PageManager::getPages(const PageIndexKey& key) const {
  std::unique_lock<std::mutex> lk(mutex_);
  Vector<PageRef> pages;

  for (const auto& p : index_) {
    if (p.key.column_id == key.column_id &&
        p.key.entry_type == key.entry_type) {
      pages.emplace_back(p.page);
    }
  }

  return pages;
}

} // namespace cstable

