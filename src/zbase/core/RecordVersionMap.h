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
#include <stx/stdtypes.h>
#include <stx/SHA1.h>

using namespace stx;

namespace zbase {

class RecordVersionMap {
public:

  static void write(
      const OrderedMap<SHA1Hash, uint64_t>& map,
      const String& filename);

  /**
   * Deletes all records from the provided map for which an entry with the same
   * id with and a version equal or greater to the provided version was found
   * in the index
   */
  static void lookup(
      HashMap<SHA1Hash, uint64_t>* map,
      const String& filename);

  static void load(
      HashMap<SHA1Hash, uint64_t>* map,
      const String& filename);

};

} // namespace zbase

