/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/fnv.h>
#include <stx/logging.h>
#include "zbase/AnalyticsTableScanSource.h"

using namespace stx;

namespace zbase {

AnalyticsTableScanSource::AnalyticsTableScanSource(
    const zbase::TSDBTableScanSpec& params,
    zbase::TSDBService* tsdb) :
    params_(params),
    tsdb_(tsdb) {}

void AnalyticsTableScanSource::read(dproc::TaskContext* context) {
  auto cstable_file = context->getDependency(0)->getData();

  cstable::CSTableReader reader(cstable_file);
  scan_.scanTable(&reader);
}

AnalyticsTableScan* AnalyticsTableScanSource::tableScan() {
  return &scan_;
}

List<dproc::TaskDependency> AnalyticsTableScanSource::dependencies() const {
  return List<dproc::TaskDependency> {
    dproc::TaskDependency {
      .task_name = "CSTableIndex",
      .params = *msg::encode(params_)
    }
  };
}

} // namespace zbase

