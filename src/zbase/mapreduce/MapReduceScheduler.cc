/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/logging.h"
#include "zbase/mapreduce/MapReduceScheduler.h"

using namespace stx;

namespace zbase {

MapReduceScheduler::MapReduceScheduler(
    const MapReduceShardList& shards) :
    shards_(shards),
    num_shards_completed_(0) {}

void MapReduceScheduler::execute() {
  logDebug(
      "z1.mapreduce",
      "Running job; progress=$0/$1",
      num_shards_completed_,
      shards_.size());
}

} // namespace zbase

