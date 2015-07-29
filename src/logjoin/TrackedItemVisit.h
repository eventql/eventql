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
#include <stx/UnixTime.h>
#include <stx/uri.h>
#include <stx/reflect/reflect.h>
#include <inventory/ItemRef.h>

namespace cm {

struct TrackedItemVisit {
  stx::UnixTime time;
  stx::String clickid;
  ItemRef item;
  std::vector<std::string> attrs;

  void merge(const TrackedItemVisit& other);
  void fromParams(const stx::URI::ParamList& params);

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&cm::TrackedItemVisit::time, 1, "t", false);
    meta->prop(&cm::TrackedItemVisit::clickid, 2, "e", false);
    meta->prop(&cm::TrackedItemVisit::item, 3, "i", false);
    meta->prop(&cm::TrackedItemVisit::attrs, 4, "a", false);
  };
};

}
#endif
