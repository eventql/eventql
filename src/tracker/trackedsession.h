/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TRACKEDSESSION_H
#define _CM_TRACKEDSESSION_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <fnord/base/datetime.h>

#include "itemref.h"
#include "trackedquery.h"

namespace cm {

struct TrackedSession {
  std::unordered_map<std::string, TrackedQuery> queries;
  std::unordered_map<std::string, TrackedItemVisit> item_visits;
  uint64_t last_access_unix_micros;
  std::mutex mutex;
  void debugPrint(const std::string& uid);
};

} // namespace cm
#endif
