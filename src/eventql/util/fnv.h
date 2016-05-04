/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _libstx_FNV_H
#define _libstx_FNV_H

#include <stdlib.h>
#include <stdint.h>
#include <string>

namespace stx {

/**
 * This implements the FNV1a (Fowler–Noll–Vo) hash function
 *   see http://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function
 */
template<typename T>
class FNV {
public:

  FNV();
  FNV(T basis, T prime) : basis_(basis), prime_(prime), memory_(basis) {}

  T hash(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
      memory_ ^= data[i];
      memory_ *= prime_;
    }

    return memory_;
  }

  inline T hash(const void* data, size_t len) {
    return hash(static_cast<const uint8_t*>(data), len);
  }

  inline T hash(const std::string& data) {
    return hash(reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
  }

  inline T get() const {
    return memory_;
  }

protected:
  T basis_;
  T prime_;
  T memory_;
};

}

#endif
