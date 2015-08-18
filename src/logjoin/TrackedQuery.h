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
#include <stx/stdtypes.h>
#include <stx/UnixTime.h>
#include <stx/uri.h>

#include <inventory/ItemRef.h>

namespace zbase {
class CustomerNamespace;

struct TrackedQueryItem {
  ItemRef item;
  bool clicked;
  bool seen;
  int position;
  int variant;
};

struct TrackedQuery {
  stx::UnixTime time;
  stx::String clickid;
  std::vector<TrackedQueryItem> items;
  std::vector<std::string> attrs;
  Set<String> experiments;

  uint32_t nitems;
  uint32_t nclicks;
  uint32_t nads;
  uint32_t nadclicks ;
  uint32_t num_cart_items;
  uint32_t num_order_items;
  uint32_t gmv_eurcents;
  uint32_t cart_value_eurcents;

  TrackedQuery();

  void merge(const TrackedQuery& other);
  void fromParams(const stx::URI::ParamList& params);
  String joinedExperiments() const;
};

} // namespace zbase
#endif
