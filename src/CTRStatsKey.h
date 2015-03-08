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
#include <fnord-base/datetime.h>
#include <fnord-base/uri.h>
#include <fnord-base/reflect/reflect.h>
#include "ItemRef.h"

using namespace fnord;

namespace cm {

typedef Vector<String> CTRStatsKey;

struct CTRStatsObservation {
  CTRStatsKey key;
  uint32_t visits;
  uint32_t clicks;
};

}
#endif
