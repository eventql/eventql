/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/util/RLEDecoder.h>

namespace fnord {
namespace util {

RLEDecoder::RLEDecoder(
    void* data,
    size_t size) :
    data_(data),
    size_(size),
    pos_(0) {}

uint32_t RLEDecoder::next() {
  uint32_t val = *((uint32_t*) (((char *) data_) + pos_));
  pos_ += sizeof(uint32_t);
  return val;
}

}
}


