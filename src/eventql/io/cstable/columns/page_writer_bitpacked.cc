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
#include <eventql/io/cstable/columns/page_writer_bitpacked.h>
#include <libsimdcomp/simdcomp.h>

namespace cstable {

BitPackedIntPageWriter::BitPackedIntPageWriter(
    PageIndexKey key,
    PageManager* page_mgr,
    uint32_t max_value /* = 0xffffffff */) :
    key_(key),
    page_mgr_(page_mgr),
    max_value_(max_value),
    inbuf_size_(0),
    maxbits_(max_value > 0 ? bits(max_value) : 0),
    has_page_(false) {}

void BitPackedIntPageWriter::appendValue(uint64_t value) {
  if (maxbits_ == 0) {
    return;
  }

  if (!has_page_) {
    page_pos_ = sizeof(max_value_);
    page_ = page_mgr_->allocPage(key_, page_pos_ + 16 * maxbits_ * kPageSize);
    page_mgr_->writeToPage(page_, 0, (const char*) &max_value_, page_pos_);
    has_page_ = true;
  } else if (page_pos_ >= page_.size) {
    page_mgr_->flushPage(page_);
    page_ = page_mgr_->allocPage(key_, 16 * maxbits_ * kPageSize);
    page_pos_ = 0;
  }

  inbuf_[inbuf_size_++] = value;

  if (inbuf_size_ == 128) {
    flush();
    inbuf_size_ = 0;
    page_pos_ += 16 * maxbits_;
  }
}

void BitPackedIntPageWriter::flush() {
  if (inbuf_size_ == 0) {
    return;
  }

  for (size_t i = inbuf_size_; i < 128; ++i) {
    inbuf_[i] = 0;
  }

  simdpackwithoutmask(inbuf_, (__m128i *) outbuf_, maxbits_);

  page_mgr_->writeToPage(
      page_,
      page_pos_,
      (const char*) outbuf_,
      16 * maxbits_);
}

} // namespace cstable

