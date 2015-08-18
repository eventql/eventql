/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TRACKEDCARTITEM_H
#define _CM_TRACKEDCARTITEM_H
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

namespace zbase {

struct TrackedCartItem {
  TrackedCartItem() :
      quantity(0),
      price_cents(0),
      checkout_step(0) {}

  stx::UnixTime time;
  ItemRef item;
  uint32_t quantity;
  uint32_t price_cents;
  String currency;
  uint32_t checkout_step;

  static Vector<TrackedCartItem> fromParams(const URI::ParamList& params);
  void merge(const TrackedCartItem& other);

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&zbase::TrackedCartItem::time, 1, "t", false);
    meta->prop(&zbase::TrackedCartItem::item, 3, "i", false);
    meta->prop(&zbase::TrackedCartItem::quantity, 4, "q", false);
    meta->prop(&zbase::TrackedCartItem::price_cents, 5, "p", false);
    meta->prop(&zbase::TrackedCartItem::currency, 6, "c", false);
    meta->prop(&zbase::TrackedCartItem::checkout_step, 7, "s", false);
  };
};

}
#endif

