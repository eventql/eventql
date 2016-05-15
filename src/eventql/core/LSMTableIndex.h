/**
 * Copyright (c) 2015 - zScale Technology GmbH <legal@zscale.io>
 *   All Rights Reserved.
 *
 * Authors:
 *   Paul Asmuth <paul@zscale.io>
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/SHA1.h>

using namespace stx;

namespace eventql {

class LSMTableIndex : public RefCounted {
public:

  static void write(
      const OrderedMap<SHA1Hash, uint64_t>& map,
      const String& filename);

  LSMTableIndex();
  LSMTableIndex(const String& filename);
  ~LSMTableIndex();

  bool load(const String& filename);

  void list(
      HashMap<SHA1Hash, uint64_t>* map);

  void lookup(
      HashMap<SHA1Hash, uint64_t>* map);

  size_t size() const;

protected:

  static const size_t kSlotSize = 28;

  inline void* getID(size_t slot) const {
    return ((char *) data_) + slot * kSlotSize;
  }

  inline uint64_t* getVersion(size_t slot) const {
    return (uint64_t*) (((char *) data_) + slot * kSlotSize + SHA1Hash::kSize);
  }

  std::mutex load_mutex_;
  size_t size_;
  void* data_;
};

} // namespace eventql

