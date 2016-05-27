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
#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <eventql/util/stdtypes.h>
#include <eventql/util/exception.h>
#include <eventql/util/buffer.h>

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
  int compare(const void* other) const;

  inline const void* data() const {
    return hash;
  }

  inline void* mutableData() {
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

  static int compare(const SHA1Hash& a, const SHA1Hash& b);
  static int compare(const void* a, const void* b);

};

namespace std {
template<>
struct hash<SHA1Hash> {
  size_t operator()(SHA1Hash const& val) const {
    static_assert(
        sizeof(size_t) < SHA1Hash::kSize,
        "sizeof(size_t must be <20 bytes");

    return *((const size_t*) val.data());
  }
};
}

