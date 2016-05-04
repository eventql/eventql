/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "eventql/mapreduce/MapReduceTask.h"
#include "stx/logging.h"

using namespace stx;

namespace zbase {

Vector<size_t> MapReduceTask::shards() const {
  return shards_;
}

size_t MapReduceTask::addShard(
    RefPtr<MapReduceTaskShard> shard,
    MapReduceShardList* shards) {
  auto idx = shards->size();
  shards_.emplace_back(idx);
  shards->emplace_back(shard);
  return idx;
}

void MapReduceJobSpec::onProgress(
    Function<void (const MapReduceJobStatus& status)> fn) {
  on_progress_ = fn;
}

void MapReduceJobSpec::updateProgress(const MapReduceJobStatus& status) {
  try {
    if (on_progress_) {
      on_progress_(status);
    }
  } catch (const StandardException& e) {
    logError("z1.mapreduce", e, "MapReduceJob on_progress callback crashed");
  }
}

void MapReduceJobSpec::onResult(
    Function<void (const String& value)> fn) {
  on_result_ = fn;
}

void MapReduceJobSpec::sendResult(const String& value) {
  try {
    if (on_result_) {
      on_result_(value);
    }
  } catch (const StandardException& e) {
    logError("z1.mapreduce", e, "MapReduceJob on_result callback crashed");
  }
}

void MapReduceJobSpec::onLogline(
    Function<void (const String& logline)> fn) {
  on_logline_ = fn;
}

void MapReduceJobSpec::sendLogline(const String& logline) {
  try {
    if (on_logline_) {
      on_logline_(logline);
    }
  } catch (const StandardException& e) {
    logError("z1.mapreduce", e, "MapReduceJob on_result callback crashed");
  }
}

} // namespace zbase

