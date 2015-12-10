/**
 * Copyright (c) 2015 - zScale Technology GmbH <legal@zscale.io>
 *   All Rights Reserved.
 *
 * Authors:
 *   Paul Asmuth <paul@zscale.io>
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <zbase/DrilldownQuery.h>

using namespace stx;

namespace zbase {

void DrilldownQuery::addMetric(MetricDefinition metric) {
  metrics_.emplace_back(metric);
}

void DrilldownQuery::addDimension(DimensionDefinition dimension) {
  dimensions_.emplace_back(dimension);
}

void DrilldownQuery::setFilter(String filter) {
  filter_ = Some(filter);
}

} // namespace zbase

