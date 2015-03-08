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
#include <fnord-base/datetime.h>
#include <fnord-base/uri.h>

#include "ItemRef.h"

namespace cm {
class CustomerNamespace;

struct TrackedQueryItem {
  ItemRef item;
  bool clicked;
  int position;
  int variant;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&cm::TrackedQueryItem::item, 1, "i", false);
    meta->prop(&cm::TrackedQueryItem::clicked, 2, "c", false);
    meta->prop(&cm::TrackedQueryItem::position, 3, "p", false);
    meta->prop(&cm::TrackedQueryItem::variant, 4, "v", false);
  };
};

struct TrackedQuery {
  fnord::DateTime time;
  fnord::String eid;
  std::vector<TrackedQueryItem> items;
  std::vector<std::string> attrs;

  void merge(const TrackedQuery& other);
  void fromParams(const fnord::URI::ParamList& params);

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&cm::TrackedQuery::time, 1, "t", false);
    meta->prop(&cm::TrackedQuery::eid, 2, "e", false);
    meta->prop(&cm::TrackedQuery::items, 3, "i", false);
    meta->prop(&cm::TrackedQuery::attrs, 4, "a", false);
  };
};

} // namespace cm
#endif
