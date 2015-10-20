/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/stdtypes.h"
#include "zbase/mapreduce/MapReduceTask.h"

using namespace stx;

namespace zbase {

class MapReduceTaskBuilder {
public:

  RefPtr<MapReduceTask> fromJSON(
      const json::JSONObject::const_iterator& begin,
      const json::JSONObject::const_iterator& end);

protected:

  RefPtr<MapReduceTask> mapTableTaskFromJSON(
      const json::JSONObject::const_iterator& begin,
      const json::JSONObject::const_iterator& end);

  RefPtr<MapReduceTask> reduceTaskFromJSON(
      const json::JSONObject::const_iterator& begin,
      const json::JSONObject::const_iterator& end);

  RefPtr<MapReduceTask> returnResultsTaskFromJSON(
      const json::JSONObject::const_iterator& begin,
      const json::JSONObject::const_iterator& end);

};

} // namespace zbase

