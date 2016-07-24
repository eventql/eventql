/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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

#ifndef _libstx_FNV_H
#define _libstx_FNV_H

#include <stdlib.h>
#include <stdint.h>
#include <string>

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

#endif
