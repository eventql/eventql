/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_PACKEDEXAMPLE_H
#define _CM_PACKEDEXAMPLE_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include "fnord-base/stdtypes.h"
#include "FeatureSchema.h"

using namespace fnord;

namespace cm {

struct Example {
  double label;
  Vector<Pair<uint64_t, double>> features;

  void sortFeatures();
};

String exampleToSVMLight(const Example& ex);

} // namespace cm

#endif
