/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_JOINEDQUERY_H
#define _CM_JOINEDQUERY_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <fnord-base/datetime.h>
#include <fnord-base/uri.h>
#include <fnord-base/reflect/reflect.h>
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "FeaturePack.h"
#include "FeatureIndex.h"
#include "fnord-mdb/MDB.h"
#include "ItemRef.h"

namespace cm {

struct JoinedQueryItem {
  ItemRef item;
  bool clicked;
  int position;
  int variant;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&cm::JoinedQueryItem::item, 1, "item", false);
    meta->prop(&cm::JoinedQueryItem::clicked, 2, "clicked", false);
    meta->prop(&cm::JoinedQueryItem::position, 3, "position", false);
    meta->prop(&cm::JoinedQueryItem::variant, 4, "variant", false);
  };
};

struct JoinedQuery {
  std::string customer;
  std::string uid;
  fnord::DateTime time;
  std::vector<JoinedQueryItem> items;
  std::vector<std::string> attrs;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&cm::JoinedQuery::customer, 1, "customer", false);
    meta->prop(&cm::JoinedQuery::uid, 2, "uid", false);
    meta->prop(&cm::JoinedQuery::time, 3, "time", false);
    meta->prop(&cm::JoinedQuery::items, 4, "items", false);
    meta->prop(&cm::JoinedQuery::attrs, 5, "attrs", false);
  };
};

}
#endif
