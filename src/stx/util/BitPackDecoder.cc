/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/util/BitPackDecoder.h>
#include <stx/exception.h>
#include <3rdparty/simdcomp/simdcomp.h>

namespace stx {
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

uint32_t BitPackDecoder::next() {
  return fetch(true);
}

uint32_t BitPackDecoder::peek() {
  return fetch(false);
}

uint32_t BitPackDecoder::fetch(bool advance) {
  if (maxbits_ == 0) {
    return 0;
  }

  if (outbuf_pos_ == 128) {
#ifndef STX_NODEBUG
    auto new_pos = pos_ + 16 * maxbits_;
    if (new_pos > size_) {
      if (!advance) {
        return 0;
      }

      RAISE(kBufferOverflowError, "read exceeds buffer boundary");
    }
#endif

    simdunpack((__m128i*) (((char *) data_) + pos_), outbuf_, maxbits_);
    pos_ = new_pos;
    outbuf_pos_ = advance ? 1 : 0;
    return outbuf_[0];
  } else {
    if (advance) {
      return outbuf_[outbuf_pos_++];
    } else {
      return outbuf_[outbuf_pos_];
    }
  }
}

}
}


