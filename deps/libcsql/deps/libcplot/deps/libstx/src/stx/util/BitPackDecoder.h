/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_UTIL_RLEDECODER_H
#define _STX_UTIL_RLEDECODER_H
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <stx/buffer.h>

namespace stx {
namespace util {

class BitPackDecoder {
public:

  BitPackDecoder(void* data, size_t size, uint32_t max_val);

  inline uint32_t next();
  inline uint32_t peek();

protected:

  void fetch();

  void* data_;
  size_t size_;
  size_t maxbits_;
  size_t pos_;
  uint32_t outbuf_[128];
  size_t outbuf_pos_;
};

inline uint32_t BitPackDecoder::next() {
  if (maxbits_ == 0) {
    return 0;
  }

  if (outbuf_pos_ == 128) {
    fetch();
  }

  return outbuf_[outbuf_pos_++];
}

inline uint32_t BitPackDecoder::peek() {
  if (maxbits_ == 0) {
    return 0;
  }

  if (outbuf_pos_ == 128) {
    fetch();
  }

  return outbuf_[outbuf_pos_];
}

}
}

#endif
