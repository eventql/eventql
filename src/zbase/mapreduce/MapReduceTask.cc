/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/mapreduce/MapReduceTask.h"
#include "zbase/mapreduce/tasks/MapTableTask.h"
#include "zbase/mapreduce/tasks/ReduceTask.h"

using namespace stx;

namespace zbase {

RefPtr<MapReduceTask> MapReduceTask::fromJSON(
    const json::JSONObject::const_iterator& begin,
    const json::JSONObject::const_iterator& end) {
  auto op = json::objectGetString(begin, end, "op");
  if (op.isEmpty()) {
    RAISE(kRuntimeError, "illegal job definition: missing op field");
  }

  if (op.get() == "map_table") {
    return MapTableTask::fromJSON(begin, end);
  }

  if (op.get() == "reduce") {
    return ReduceTask::fromJSON(begin, end);
  }

  RAISEF(kRuntimeError, "unknown operation: $0", op.get());
}


} // namespace zbase

