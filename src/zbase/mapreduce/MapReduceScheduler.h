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

class MapReduceScheduler {
public:

  MapReduceScheduler(const MapReduceShardList& shards);

  void execute();

protected:
  MapReduceShardList shards_;
  size_t num_shards_completed_;
};

} // namespace zbase

