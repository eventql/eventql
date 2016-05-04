/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ITEMREF_H
#define _CM_ITEMREF_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include "stx/reflect/reflect.h"
#include "DocID.h"

using namespace stx;

namespace zbase {

struct ItemRef {
  std::string set_id;
  std::string item_id;

  bool operator==(const ItemRef& other) const;

  DocID docID() const;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&ItemRef::set_id, 1, "set_id", false);
    meta->prop(&ItemRef::item_id, 2, "item_id", false);
  }
};

struct ItemRefWithPosition {
  std::string set_id;
  std::string item_id;
  int position;

  bool operator==(const ItemRef& other) const;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&ItemRefWithPosition::set_id, 1, "set_id", false);
    meta->prop(&ItemRefWithPosition::item_id, 2, "item_id", false);
    meta->prop(&ItemRefWithPosition::position, 3, "position", true);
  }
};

} // namespace zbase

#endif
