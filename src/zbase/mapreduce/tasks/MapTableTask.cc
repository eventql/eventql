/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/mapreduce/tasks/MapTableTask.h"

using namespace stx;

namespace zbase {

RefPtr<MapReduceTask> MapTableTask::fromJSON(
    const json::JSONObject::const_iterator& begin,
    const json::JSONObject::const_iterator& end) {
  RAISE(kNotYetImplementedError, "not yet implemented");
}


} // namespace zbase

