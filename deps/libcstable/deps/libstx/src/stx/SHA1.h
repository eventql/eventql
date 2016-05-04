/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_UTIL_SHA1_H
#define _STX_UTIL_SHA1_H
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <stx/stdtypes.h>
#include <stx/exception.h>
#include <stx/buffer.h>

namespace stx {

class SHA1Hash {
  friend class SHA1;
public:
  static const size_t kSize = 20;
  struct DeferInitialization {};

  /**
   * Parse a SHA1Hash from a hex encoded string
   */
  static SHA1Hash fromHexString(const String& str);

  /**
   * Creates a new zero-initialized SHA1 hash
   */
  SHA1Hash();

  /**
   * Creates an unitialized SHA1 hash. The initial value is undefined.
   */
  SHA1Hash(DeferInitialization);

  /**
   * Creates a new SHA1 hash from an already computed hash. size must be 20
   * bytes.
   */
  SHA1Hash(const void* data, size_t size);

  bool operator==(const SHA1Hash& other) const;
  bool operator<(const SHA1Hash& other) const;
  bool operator>(const SHA1Hash& other) const;

  int compare(const SHA1Hash& other) const;

  inline const void* data() const {
    return hash;
  }

  inline size_t size() const {
    return sizeof(hash);
  }

  String toString() const;

protected:
  uint8_t hash[kSize];
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
