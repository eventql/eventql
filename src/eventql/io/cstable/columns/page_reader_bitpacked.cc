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
#include <eventql/io/cstable/columns/page_reader_bitpacked.h>
#include <libsimdcomp/simdcomp.h>
#include <assert.h>

namespace cstable {

BitPackedIntPageReader::BitPackedIntPageReader(
    PageIndexKey key,
    const PageManager* page_mgr) :
    page_mgr_(page_mgr),
    pages_(page_mgr->getPages(key)),
    page_pos_(0),
    page_len_(0),
    page_idx_(0),
    eof_(false),
    outbuf_pos_(128) {
  if (pages_.size() > 0) {
    fetchNextPage();
    auto max_val = *page_data_.structAt<uint32_t>(0);
    page_pos_ = sizeof(uint32_t);
    maxbits_ = max_val > 0 ? bits(max_val) : 0;
    fetchNext();
  } else {
    eof_ = true;
  }
}

uint64_t BitPackedIntPageReader::readUnsignedInt() {
  if (eof_) {
    RAISE(kRuntimeError, "end of column reached");
  }

  auto val = cur_val_;
  fetchNext();
  return val;
}

void BitPackedIntPageReader::fetchNext() {
  if (eof_) {
    return;
  }

  if (maxbits_ == 0) {
    cur_val_ = 0;
    return;
  }

  if (outbuf_pos_ == 128) {
    fetchNextBatch();
  }

  cur_val_ = outbuf_[outbuf_pos_++];
}

void BitPackedIntPageReader::fetchNextPage() {
  page_pos_ = 0;
  assert(page_idx_ < pages_.size());
  page_len_ = pages_[page_idx_].size;
  page_data_.resize(page_len_);
  page_mgr_->readPage(pages_[page_idx_], page_data_.data());
  ++page_idx_;
}

void BitPackedIntPageReader::fetchNextBatch() {
  auto batch_size = 16 * maxbits_;
  uint8_t batch_data[512];

  for (size_t b = 0; b < batch_size; ) {
    if (page_pos_ == page_len_) {
      if (page_idx_ < pages_.size()) {
        fetchNextPage();
      } else {
        break;
      }
    }

    auto c = std::min(
        uint32_t(page_len_ - page_pos_),
        uint32_t(batch_size - b));

    memcpy(&batch_data[b], page_data_.structAt<void>(page_pos_), c);
    page_pos_ += c;
    b += c;
  }

  simdunpack((__m128i*) batch_data, outbuf_, maxbits_);
  outbuf_pos_ = 0;
}

uint64_t BitPackedIntPageReader::peek() {
  return cur_val_;
}

bool BitPackedIntPageReader::eofReached() {
  return eof_;
}

void BitPackedIntPageReader::rewind() {
  page_pos_ = 0;
  page_len_ = 0;
  page_idx_ = 0;
  eof_ = false;
  outbuf_pos_ = 128;

  if (pages_.size() > 0) {
    fetchNextPage();
    auto max_val = *page_data_.structAt<uint32_t>(0);
    page_pos_ = sizeof(uint32_t);
    maxbits_ = max_val > 0 ? bits(max_val) : 0;
    fetchNext();
  } else {
    eof_ = true;
  }
}

} // namespace cstable

