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
#include <3rdparty/simdcomp/simdcomp.h>
#include <eventql/util/inspect.h>
#include <eventql/util/util/BitPackEncoder.h>

namespace stx {
namespace util {


BitPackEncoder::BitPackEncoder(
    uint32_t max_val) :
    maxbits_(max_val > 0 ? bits(max_val) : 0),
    inbuf_size_(0) {}

void BitPackEncoder::encode(uint32_t value) {
  if (maxbits_ == 0) {
    return;
  }

  inbuf_[inbuf_size_++] = value;

  if (inbuf_size_ == 128) {
    flush();
  }
}

void BitPackEncoder::flush() {
  if (inbuf_size_ == 0) {
    return;
  }

  while (inbuf_size_ < 128) {
    inbuf_[inbuf_size_++] = 0;
  }

  simdpackwithoutmask(inbuf_, (__m128i *) outbuf_, maxbits_);
  buf_.append(outbuf_, 16 * maxbits_);
  inbuf_size_ = 0;
}

void* BitPackEncoder::data() const {
  return buf_.data();
}

size_t BitPackEncoder::size() const {
  return buf_.size();
}

}
}

