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
#ifndef _STX_UTIL_RLEDECODER_H
#define _STX_UTIL_RLEDECODER_H
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <eventql/util/buffer.h>

namespace util {

class BitPackDecoder {
public:

  BitPackDecoder(void* data, size_t size, uint32_t max_val);

  inline uint32_t next();
  inline uint32_t peek();

  void rewind();

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

#endif
