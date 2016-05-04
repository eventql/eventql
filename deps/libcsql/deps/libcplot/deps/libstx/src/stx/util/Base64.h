/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_UTIL_BASE64_H
#define _STX_UTIL_BASE64_H
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <stx/buffer.h>

namespace stx {
namespace util {

class Base64 {
public:

  static void encode(const String& in, String* out);
  static void encode(const void* data, size_t size, String* out);
  static String encode(const String& in);
  static String encode(const void* data, size_t size);

  static void decode(const String& in, String* out);

};

}
}

#endif
