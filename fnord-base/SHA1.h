/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_UTIL_SHA1_H
#define _FNORD_UTIL_SHA1_H
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <fnord-base/stdtypes.h>
#include <fnord-base/buffer.h>

namespace fnord {

struct SHA1Hash {
  uint8_t hash[20];
  String toString() const;
};

class SHA1 {
public:

  static SHA1Hash compute(const Buffer& data);
  static void compute(const Buffer& data, SHA1Hash* out);

  static SHA1Hash compute(const String& data);
  static void compute(const String& data, SHA1Hash* out);

  static SHA1Hash compute(const void* data, size_t size);
  static void compute(const void* data, size_t size, SHA1Hash* out);

};

}

#endif
