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
#include <eventql/io/cstable/columns/page_writer_uint64.h>
#include <eventql/util/inspect.h>


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


