/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include "eventql/mapreduce/MapReduceTask.h"
#include "eventql/util/logging.h"

using namespace util;

namespace eventql {

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

} // namespace eventql

