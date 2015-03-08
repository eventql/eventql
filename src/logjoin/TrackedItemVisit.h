/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TRACKEDITEMVISIT_H
#define _CM_TRACKEDITEMVISIT_H
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

namespace cm {

struct TrackedItemVisit {
  fnord::DateTime time;
  fnord::String eid;
  ItemRef item;
  std::vector<std::string> attrs;

  void merge(const TrackedItemVisit& other);
  void fromParams(const fnord::URI::ParamList& params);

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&cm::TrackedItemVisit::time, 1, "t", false);
    meta->prop(&cm::TrackedItemVisit::eid, 2, "e", false);
    meta->prop(&cm::TrackedItemVisit::item, 3, "i", false);
    meta->prop(&cm::TrackedItemVisit::attrs, 4, "a", false);
  };
};

}
#endif
