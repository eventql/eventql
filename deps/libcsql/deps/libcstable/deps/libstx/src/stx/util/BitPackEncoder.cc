/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <3rdparty/simdcomp/simdcomp.h>
#include <stx/inspect.h>
#include <stx/util/BitPackEncoder.h>

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

