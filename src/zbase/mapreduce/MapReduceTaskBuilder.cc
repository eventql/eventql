/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/mapreduce/MapReduceTaskBuilder.h"
#include "zbase/mapreduce/tasks/MapTableTask.h"
#include "zbase/mapreduce/tasks/ReduceTask.h"

using namespace stx;

namespace zbase {

RefPtr<MapReduceTask> MapReduceTaskBuilder::fromJSON(
    const json::JSONObject::const_iterator& begin,
    const json::JSONObject::const_iterator& end) {
  auto op = json::objectGetString(begin, end, "op");
  if (op.isEmpty()) {
    RAISE(kRuntimeError, "illegal job definition: missing op field");
  }

  if (op.get() == "map_table") {
    return mapTableTaskFromJSON(begin, end);
  }

  if (op.get() == "reduce") {
    return reduceTaskFromJSON(begin, end);
  }

  RAISEF(kRuntimeError, "unknown operation: $0", op.get());
}

RefPtr<MapReduceTask> MapReduceTaskBuilder::mapTableTaskFromJSON(
    const json::JSONObject::const_iterator& begin,
    const json::JSONObject::const_iterator& end) {
  return new MapTableTask();
}

RefPtr<MapReduceTask> MapReduceTaskBuilder::reduceTaskFromJSON(
    const json::JSONObject::const_iterator& begin,
    const json::JSONObject::const_iterator& end) {
  auto src_begin = json::objectLookup(begin, end, "sources");
  if (src_begin == end) {
    RAISE(kRuntimeError, "missing field: sources");
  }

  Vector<RefPtr<MapReduceTask>> sources;
  auto nsrc_begin = json::arrayLength(src_begin, end);
  for (size_t i = 0; i < nsrc_begin; ++i) {
    auto src = json::arrayLookup(src_begin, end, i); // O(N^2) but who cares...
    sources.emplace_back(fromJSON(src, src + src->size));
  }

  return new ReduceTask(sources);
}

} // namespace zbase

