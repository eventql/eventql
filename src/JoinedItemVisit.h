/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_JOINEDITEMVISIT_H
#define _CM_JOINEDITEMVISIT_H
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

struct JoinedItemVisit {
  std::string customer;
  std::string uid;
  DateTime time;
  ItemRef item;
  std::vector<std::string> attrs;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&cm::JoinedItemVisit::customer, 1, "customer", false);
    meta->prop(&cm::JoinedItemVisit::uid, 2, "uid", false);
    meta->prop(&cm::JoinedItemVisit::time, 3, "time", false);
    meta->prop(&cm::JoinedItemVisit::item, 4, "item", false);
    meta->prop(&cm::JoinedItemVisit::attrs, 5, "attrs", false);
  };
};

}
#endif
