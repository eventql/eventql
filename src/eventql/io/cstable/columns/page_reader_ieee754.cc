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
#include <eventql/io/cstable/columns/page_reader_ieee754.h>
#include <eventql/util/ieee754.h>

namespace cstable {

IEEE754PageReader::IEEE754PageReader(
    PageIndexKey key,
    const PageManager* page_mgr) :
    page_mgr_(page_mgr),
    pages_(page_mgr->getPages(key)),
    page_pos_(0),
    page_len_(0),
    page_idx_(0) {}

double IEEE754PageReader::readFloat() {
  if (page_pos_ + sizeof(uint64_t) > page_len_) {
    if (page_idx_ == pages_.size()) {
      return 0;
    }

    page_pos_ = 0;
    page_len_ = pages_[page_idx_].size;
    page_data_.resize(page_len_);
    page_mgr_->readPage(pages_[page_idx_], page_data_.data());
    ++page_idx_;
  }

  uint64_t v;
  memcpy(
      &v,
      (const char*) page_data_.data() + page_pos_,
      sizeof(uint64_t));

  page_pos_ += sizeof(uint64_t);
  return IEEE754::fromBytes(v);
}

void IEEE754PageReader::rewind() {
  page_pos_ = 0;
  page_len_ = 0;
  page_idx_ = 0;
}

} // namespace cstable

