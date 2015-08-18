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
#include "common.h"
#include "logjoin/TrackedItemVisit.h"

namespace zbase {

void TrackedItemVisit::fromParams(const stx::URI::ParamList& params) {
  std::string item_id_str;
  if (!stx::URI::getParam(params, "i", &item_id_str)) {
    RAISE(kParseError, "invalid i param");
  }

  auto item_id_parts = stx::StringUtil::split(item_id_str, "~");
  if (item_id_parts.size() < 2) {
    RAISE(kParseError, "invalid i param");
  }

  item.set_id = item_id_parts[0];
  item.item_id = item_id_parts[1];

  /* extract all non-reserved params as event attributes */
  for (const auto& p : params) {
    if (!isReservedPixelParam(p.first)) {
      attrs.emplace_back(stx::StringUtil::format("$0:$1", p.first, p.second));
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

