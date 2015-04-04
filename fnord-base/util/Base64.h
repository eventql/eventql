/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_UTIL_BASE64_H
#define _FNORD_UTIL_BASE64_H
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <fnord-base/buffer.h>

namespace fnord {
namespace util {

class Base64 {
public:

  static void encode(const String& in, String* out);
  static void decode(const String& in, String* out);

};

}
}

#endif
