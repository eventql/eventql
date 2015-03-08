/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "FeatureCache.h"

using namespace fnord;

namespace cm {

Option<String> FeatureCache::get(const String& key) {
  auto iter = data_.find(key);

  if (iter == data_.end()) {
    return None<String>();
  } else {
    return Some(iter->second);
  }
}

void FeatureCache::store(const String& key, const String& value) {
  data_[key] = value;
}

} // namespace cm
