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
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <zbase/DrilldownTree.h>

using namespace stx;

namespace zbase {

class DrilldownQuery : public RefCounted {
public:

  struct MetricDefinition {
    String name;
    String expression;
    Option<String> source_table;
    Option<String> filter;
  };

  struct DimensionDefinition {
    String name;
    String expression;
    Option<String> order;
  };

  void addMetric(MetricDefinition metric);

  void addDimension(DimensionDefinition dimension);

  void setFilter(String filter);

protected:
  Vector<MetricDefinition> metrics_;
  Vector<DimensionDefinition> dimensions_;
  Option<String> filter_;
  DrilldownTree dtree_;
};

} // namespace zbase

