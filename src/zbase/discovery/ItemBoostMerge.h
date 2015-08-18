/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ITEMBOOSTMERGE_H
#define _CM_ITEMBOOSTMERGE_H
#include <stx/stdtypes.h>
#include "zbase/ItemBoost.pb.h"

using namespace stx;

namespace zbase {

struct ItemBoostMerge {
  static void merge(ItemBoostRow* dst, const ItemBoostRow& src);
};

} // namespace zbase

#endif
