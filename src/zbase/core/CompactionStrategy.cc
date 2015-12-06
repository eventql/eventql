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
#include <zbase/core/CompactionStrategy.h>

using namespace stx;

namespace zbase {

bool SimpleCompactionStrategy::compact(
    const Vector<LSMTableRef>& input,
    Vector<LSMTableRef>* output) {
  *output = input;
  return true;
}

bool SimpleCompactionStrategy::needsCompaction(
    const Vector<LSMTableRef>& tables) {
  return tables.size() > 1;
}

} // namespace zbase

