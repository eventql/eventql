/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "trackedquery.h"

namespace cm {

void TrackedQuery::merge(const TrackedQuery& other) {
  for (const auto& attr : other.attrs) {
    if (std::find(attrs.begin(), attrs.end(), attr) == attrs.end()) {
      attrs.emplace_back(attr);
    }
  }

  for (const auto& other_item : other.items) {
    bool found = false;

    for (const auto& this_item : items) {
      if (other_item.item == this_item.item &&
          other_item.position == this_item.position &&
          other_item.variant == this_item.variant) {
        found = true;
        break;
      }
    }

    if (found == false) {
      items.emplace_back(other_item);
    }
  }
}

void TrackedItemVisit::merge(const TrackedItemVisit& other) {
  for (const auto& attr : other.attrs) {
    if (std::find(attrs.begin(), attrs.end(), attr) == attrs.end()) {
      attrs.emplace_back(attr);
    }
  }
}

}

