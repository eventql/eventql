/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <algorithm>
#include <stx/exception.h>
#include <stx/inspect.h>
#include <stx/stringutil.h>
#include <stx/logging.h>
#include <stx/uri.h>
#include "common.h"
#include "zbase/logjoin/TrackedCartItem.h"

namespace zbase {

Vector<TrackedCartItem> TrackedCartItem::fromParams(
  const stx::URI::ParamList& params) {
  Vector<TrackedCartItem> items;

  try {
    std::string step_str;
    if (!stx::URI::getParam(params, "s", &step_str)) {
      RAISE(kParseError, "missing s param");
    }

    auto checkout_step = std::stoul(step_str);

    std::string items_str;
    if (stx::URI::getParam(params, "is", &items_str)) {
      for (const auto& item_str : stx::StringUtil::split(items_str, ",")) {
        if (item_str.length() == 0) {
          continue;
        }

        auto item_str_parts = stx::StringUtil::split(item_str, "~");
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
  } catch (const StandardException& e) {
    logError("logjoin", e, "error while parsing TrackedCartItem");
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

