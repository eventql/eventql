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
#include <fnord/base/datetime.h>
#include <fnord/base/uri.h>
#include <fnord/reflect/reflect.h>
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

  void merge(const TrackedQuery& other);
  void fromParams(const fnord::URI::ParamList& params);
};

struct TrackedItemVisit {
  fnord::DateTime time;
  ItemRef item;
  std::vector<std::string> attrs;

  void merge(const TrackedItemVisit& other);
  void fromParams(const fnord::URI::ParamList& params);
};
}

template <> template <class T>
void fnord::reflect::MetaClass<
    cm::TrackedItemVisit>::reflect(T* t) {
  t->prop(&cm::TrackedItemVisit::time, 1, "time", false);
  t->prop(&cm::TrackedItemVisit::item, 2, "item", false);
  t->prop(&cm::TrackedItemVisit::attrs, 3, "attrs", false);
}
#endif
