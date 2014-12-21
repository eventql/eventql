/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TRACKEDQUERY_H
#define _CM_TRACKEDQUERY_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "itemref.h"

namespace cm {

struct TrackedQueryItem {
  ItemRef item;
  bool clicked;
  int position;
  int variant;
};

struct TrackedQuery {
  fnord::DateTime time;
  std::vector<TrackedQueryItem> items;
  std::vector<std::string> attrs;
  bool flushed;
};

struct TrackedItemVisit {
  fnord::DateTime time;
  ItemRef item;
  std::vector<std::string> attrs;
};

}
#endif
