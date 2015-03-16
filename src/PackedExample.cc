/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <algorithm>
#include "fnord-base/stringutil.h"
#include "PackedExample.h"

using namespace fnord;

namespace cm {

void Example::sortFeatures() {
  std::sort(features.begin(), features.end(), [] (
      const Pair<uint64_t, double>& a,
      const Pair<uint64_t, double>& b) {
    return a.first < b.first;
  });
}

String exampleToSVMLight(const Example& ex) {
  auto str = StringUtil::toString(ex.label);

  int n = 0;
  for (const auto& f : ex.features) {
    str += StringUtil::format(" $0:$1", f.first, f.second);
    ++n;
  }

  if (n == 0) {
    return "";
  } else {
    return str;
  }
}

} // namespace cm
