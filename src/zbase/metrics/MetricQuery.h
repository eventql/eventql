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
#include "stx/autoref.h"
#include "stx/option.h"
#include "stx/json/json.h"
#include "stx/autoref.h"
#include "zbase/core/ReplicationScheme.h"
#include "zbase/metrics/MetricDefinition.pb.h"

using namespace stx;

namespace zbase {

class MetricQuery : public RefCounted {
public:

  void addMetric(const MetricDefinition& metric);

  void onProgress(Function<void (double progress)> fn);
  void updateProgress(double progress);

protected:
  Function<void (double progress)> on_progress_;
  Vector<MetricDefinition> metrics_;
};

} // namespace zbase

