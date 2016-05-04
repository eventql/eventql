/**
 * This file is part of the "libcstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libcstable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/columns/UInt64PageWriter.h>
#include <stx/inspect.h>


namespace cstable {

UInt64PageWriter::UInt64PageWriter(
    PageIndexKey key,
    RefPtr<PageManager> page_mgr,
    RefPtr<PageIndex> page_idx) :
    key_(key),
    page_mgr_(page_mgr),
    has_page_(false),
    page_os_(&page_buf_) {
  page_idx->addPageWriter(key_, this);
}

void UInt64PageWriter::appendValue(uint64_t value) {
  if (!has_page_ || page_buf_.size() + sizeof(uint64_t) > page_.size) {
    if (has_page_) {
      page_mgr_->writePage(page_, page_buf_);
      pages_.emplace_back(page_, page_buf_.size());
    }

    page_ = page_mgr_->allocPage(kPageSize);
    if (page_buf_.size() < page_.size) {
      page_buf_.reserve(page_.size - page_buf_.size());
    }

    has_page_ = true;
  }

  page_os_.appendUInt64(value);
}

void UInt64PageWriter::writeIndex(OutputStream* os) const {
  auto pages = pages_;
  if (has_page_) {
    page_mgr_->writePage(page_, page_buf_);
    pages.emplace_back(page_, page_buf_.size());
  }

  os->appendVarUInt(pages.size());
  for (const auto& p : pages) {
    os->appendVarUInt(p.first.offset);
    os->appendVarUInt(p.first.size);
    os->appendVarUInt(p.second);
  }
}


} // namespace cstable


