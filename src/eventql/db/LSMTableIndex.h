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
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/SHA1.h>

#include "eventql/eventql.h"

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

