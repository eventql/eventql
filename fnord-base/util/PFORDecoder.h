/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_UTIL_PFORDECODER_H
#define _FNORD_UTIL_PFORDECODER_H
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <fnord-base/Buffer.h>

namespace fnord {
namespace util {

class PFORDecoder {
public:

  PFORDecoder(void* data, size_t size);

  uint32_t next();

protected:
  void* data_;
  size_t size_;
  size_t pos_;
};

}
}

#endif
