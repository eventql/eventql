/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/util/BitPackDecoder.h>
#include <fnord-base/exception.h>
#include <3rdparty/simdcomp/simdcomp.h>

namespace fnord {
namespace util {

BitPackDecoder::BitPackDecoder(
    void* data,
    size_t size,
    uint32_t max_val) :
    data_(data),
    size_(size),
    maxbits_(bits(max_val)),
    pos_(0),
    outbuf_pos_(128) {}

uint32_t BitPackDecoder::next() {
  if (outbuf_pos_ == 128) {
#ifndef FNORD_NODEBUG
    auto new_pos = pos_ + 16 * maxbits_;
    if (new_pos > size_) {
      RAISE(kBufferOverflowError, "read exceeds buffer boundary");
    }
#endif

    simdunpack((__m128i*) (((char *) data_) + pos_), outbuf_, maxbits_);
    pos_ = new_pos;
    outbuf_pos_ = 1;
    return outbuf_[0];
  } else {
    return outbuf_[outbuf_pos_++];
  }
}

}
}


