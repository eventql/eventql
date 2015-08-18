/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRSTATSKEY_H
#define _CM_CTRSTATSKEY_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <stx/UnixTime.h>
#include <stx/uri.h>
#include <stx/reflect/reflect.h>
#include <inventory/ItemRef.h>

using namespace stx;

namespace zbase {

typedef Vector<String> CTRStatsKey;

struct CTRStatsObservation {
  CTRStatsKey key;
  uint32_t visits;
  uint32_t clicks;
};

}
#endif
