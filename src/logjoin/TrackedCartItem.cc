/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <algorithm>
#include <fnord/exception.h>
#include <fnord/inspect.h>
#include <fnord/stringutil.h>
#include <fnord/uri.h>
#include "common.h"
#include "logjoin/TrackedCartItem.h"

namespace cm {

Vector<TrackedCartItem> TrackedCartItem::fromParams(
  const fnord::URI::ParamList& params) {
  Vector<TrackedCartItem> items;

  std::string step_str;
  if (!fnord::URI::getParam(params, "s", &step_str)) {
    RAISE(kParseError, "missing s param");
  }

  auto checkout_step = std::stoul(step_str);

  std::string items_str;
  if (fnord::URI::getParam(params, "is", &items_str)) {
    for (const auto& item_str : fnord::StringUtil::split(items_str, ",")) {
      if (item_str.length() == 0) {
        continue;
      }

      auto item_str_parts = fnord::StringUtil::split(item_str, "~");
      if (item_str_parts.size() != 5) {
        RAISE(kParseError, "invalid is param");
      }

      TrackedCartItem citem;
      citem.item.set_id = item_str_parts[0];
      citem.item.item_id = item_str_parts[1];
      citem.quantity = std::stoul(item_str_parts[2]);
      citem.price_cents = std::stoul(item_str_parts[3]);
      citem.currency = item_str_parts[4];
      citem.checkout_step = checkout_step;

      items.emplace_back(citem);
    }
  }

  return items;
}

void TrackedCartItem::merge(const TrackedCartItem& other) {
  if (other.time > time) {
    quantity = other.quantity;
  }

  if ((other.checkout_step > 0 && other.checkout_step < checkout_step) ||
      checkout_step == 0) {
    checkout_step = other.checkout_step;
  }
}

}

