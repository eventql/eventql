/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/logging.h"
#include "zbase/metrics/MetricQuery.h"

using namespace stx;

namespace zbase {

void MetricQuery::onProgress(Function<void (double progress)> fn) {
  on_progress_ = fn;
}

void MetricQuery::updateProgress(double progress) {
  try {
    if (on_progress_) {
      on_progress_(progress);
    }
  } catch (const StandardException& e) {
    logError("z1.metrics", e, "MetricQuery on_progress callback crashed");
  }
}

} // namespace zbase

