/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_INDEXREQUEST_H
#define _CM_INDEXREQUEST_H
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

struct IndexChangeRequest {
  String customer;
  ItemRef item;
  HashMap<String, String> attrs;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&IndexChangeRequest::customer, 1, "customer", false);
    meta->prop(&IndexChangeRequest::item, 2, "docid", false);
    meta->prop(&IndexChangeRequest::attrs, 3, "attributes", false);
  };
};

}
#endif
