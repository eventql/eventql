/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/mapreduce/MapReduceTask.h"
#include "stx/logging.h"

using namespace stx;

namespace zbase {

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
    Function<void (const String& key, const String& value)> fn) {
  on_result_ = fn;
}

void MapReduceJobSpec::sendResult(const String& key, const String& value) {
  try {
    if (on_result_) {
      on_result_(key, value);
    }
  } catch (const StandardException& e) {
    logError("z1.mapreduce", e, "MapReduceJob on_result callback crashed");
  }
}

} // namespace zbase

