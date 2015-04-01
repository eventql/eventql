/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_UTIL_PFORENCODER_H
#define _FNORD_UTIL_PFORENCODER_H
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <fnord-base/Buffer.h>

namespace fnord {
namespace util {

class PFOREncoder {
public:

  void encode(uint32_t value);
  void flush();

  void* data() const;
  size_t size() const;

protected:
  Buffer buf_;
};

}
}

#endif
