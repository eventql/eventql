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
#include <stx/logging.h>
#include <stx/stringutil.h>
#include <stx/uri.h>
#include "common.h"
#include "logjoin/TrackedQuery.h"

namespace zbase {

TrackedQuery::TrackedQuery() :
    nitems(0),
    nclicks(0),
    nads(0),
    nadclicks (0),
    num_cart_items(0),
    num_order_items(0),
    gmv_eurcents(0),
    cart_value_eurcents(0) {}

void TrackedQuery::fromParams(const stx::URI::ParamList& params) {

  std::string items_str;
  if (stx::URI::getParam(params, "is", &items_str)) {
    for (const auto& item_str : stx::StringUtil::split(items_str, ",")) {
      try {
        if (item_str.length() == 0) {
          continue;
        }

        auto item_str_parts = stx::StringUtil::split(item_str, "~");
        if (item_str_parts.size() < 2) {
          RAISE(kParseError, "invalid is param");
        }

        TrackedQueryItem qitem;
        qitem.item.set_id = item_str_parts[0];
        qitem.item.item_id = item_str_parts[1];
        qitem.clicked = false;
        qitem.seen = false;
        qitem.position = -1;
        qitem.variant = -1;

        for (int i = 2; i < item_str_parts.size(); ++i) {
          const auto& iattr = item_str_parts[i];
          if (iattr.length() < 1) {
            continue;
          }

          switch (iattr[0]) {
            case 'p':
              qitem.position = std::stoi(iattr.substr(1, iattr.length() - 1));
              break;
            case 'v':
              qitem.variant = std::stoi(iattr.substr(1, iattr.length() - 1));
              break;
            case 's':
              qitem.seen = true;
              break;
          }
        }

        items.emplace_back(qitem);
      } catch (const StandardException& e) {
        logError("logjoin", e, "error while parsing TrackedQuery::ResultItem");
      }
    }
  }

  /* extract all non-reserved params as event attributes */
  for (const auto& p : params) {
    if (p.first == "qx") {
      experiments.emplace(p.second);
      continue;
    }

    if (!isReservedPixelParam(p.first)) {
      attrs.emplace_back(stx::StringUtil::format("$0:$1", p.first, p.second));
    }
  }
}

void TrackedQuery::merge(const TrackedQuery& other) {
  for (const auto& attr : other.attrs) {
    if (std::find(attrs.begin(), attrs.end(), attr) == attrs.end()) {
      attrs.emplace_back(attr);
    }
  }

  for (const auto& exp : other.experiments) {
    experiments.emplace(exp);
  }

  for (const auto& other_item : other.items) {
    bool found = false;

    for (auto& this_item : items) {
      if (other_item.item == this_item.item &&
          (
              other_item.position == this_item.position ||
              other_item.position == -1 ||
              this_item.position == -1
          ) &&
          other_item.variant == this_item.variant) {
        this_item.seen = this_item.seen || other_item.seen;
        found = true;
        break;
      }
    }

    if (found == false) {
      items.emplace_back(other_item);
    }
  }
}

String TrackedQuery::joinedExperiments() const {
  String joined;

  for (const auto& e : experiments) {
    joined += e;
    joined += ';';
  }

  return joined;
}

}

