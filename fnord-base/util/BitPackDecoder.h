/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_UTIL_RLEDECODER_H
#define _FNORD_UTIL_RLEDECODER_H
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <fnord-base/buffer.h>

namespace fnord {
namespace util {

class BitPackDecoder {
public:

  BitPackDecoder(void* data, size_t size, uint32_t max_val);

  uint32_t next();

protected:
  void* data_;
  size_t size_;
  size_t maxbits_;
  size_t pos_;
  uint32_t outbuf_[128];
  size_t outbuf_pos_;
};

}
}

#endif
