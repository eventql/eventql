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
#include <eventql/io/cstable/columns/page_reader_lenencstring.h>

namespace cstable {

LenencStringPageReader::LenencStringPageReader(
    PageIndexKey key,
    const PageManager* page_mgr) :
    page_mgr_(page_mgr),
    pages_(page_mgr->getPages(key)),
    page_pos_(0),
    page_len_(0),
    page_idx_(0) {}

void LenencStringPageReader::readString(String* str) {
  uint64_t strlen = 0;
  for (int i = 0; ; ++i) {
    if (page_pos_ >= page_len_) {
      loadNextPage();
    }

    unsigned char b = ((unsigned char*) page_data_.data())[page_pos_++];
    strlen |= (b & 0x7fULL) << (7 * i);

    if (!(b & 0x80U)) {
      break;
    }
  }

  while (strlen) {
    if (page_pos_ >= page_len_) {
      loadNextPage();
    }

    auto read_len = std::min(strlen, page_len_ - page_pos_);
    *str += String((const char*) page_data_.data() + page_pos_, read_len);
    page_pos_ += read_len;
    strlen -= read_len;
  }
}

void LenencStringPageReader::loadNextPage() {
  if (page_idx_ == pages_.size()) {
    RAISE(kRuntimeError, "end of column reached");
  }

  page_pos_ = 0;
  page_len_ = pages_[page_idx_].size;
  page_data_.resize(page_len_);
  page_mgr_->readPage(pages_[page_idx_], page_data_.data());
  ++page_idx_;
}

void LenencStringPageReader::rewind() {
  page_pos_ = 0;
  page_len_ = 0;
  page_idx_ = 0;
}

} // namespace cstable

