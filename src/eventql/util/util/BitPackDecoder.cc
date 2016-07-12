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
#include <eventql/util/util/BitPackDecoder.h>
#include <eventql/util/exception.h>
#include <libsimdcomp/simdcomp.h>

namespace util {

BitPackDecoder::BitPackDecoder(
    void* data,
    size_t size,
    uint32_t max_val) :
    data_(data),
    size_(size),
    maxbits_(max_val > 0 ? bits(max_val) : 0),
    pos_(0),
    outbuf_pos_(128) {}

void BitPackDecoder::rewind() {
  pos_ = 0;
  outbuf_pos_ = 128;
}

void BitPackDecoder::fetch() {
  auto new_pos = pos_ + 16 * maxbits_;
  simdunpack((__m128i*) (((char *) data_) + pos_), outbuf_, maxbits_);
  pos_ = new_pos;
  outbuf_pos_ = 0;
}

}


