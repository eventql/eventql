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
      const HashMap<SHA1Hash, uint64_t>& map,
      const String& filename);

  static uint64_t lookup(
      const SHA1Hash& id,
      const String& filename);


};

} // namespace zbase

