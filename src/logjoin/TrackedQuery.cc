/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <algorithm>
#include <fnord-base/exception.h>
#include <fnord-base/inspect.h>
#include <fnord-base/stringutil.h>
#include <fnord-base/uri.h>
#include "common.h"
#include "logjoin/TrackedQuery.h"

namespace cm {

void TrackedQuery::fromParams(const fnord::URI::ParamList& params) {

  std::string items_str;
  if (fnord::URI::getParam(params, "is", &items_str)) {
    for (const auto& item_str : fnord::StringUtil::split(items_str, ",")) {
      if (item_str.length() == 0) {
        continue;
      }

      auto item_str_parts = fnord::StringUtil::split(item_str, "~");
      if (item_str_parts.size() < 2) {
        RAISE(kParseError, "invalid is param");
      }

      TrackedQueryItem qitem;
      qitem.item.set_id = item_str_parts[0];
      qitem.item.item_id = item_str_parts[1];
      qitem.clicked = false;
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
        }
      }

      items.emplace_back(qitem);
    }
  }

  /* extract all non-reserved params as event attributes */
  for (const auto& p : params) {
    if (!isReservedPixelParam(p.first)) {
      attrs.emplace_back(fnord::StringUtil::format("$0:$1", p.first, p.second));
    }
  }
}

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


}

