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
#include "csql/runtime/ExecutionStrategy.h"

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

  DrilldownQuery(
      RefPtr<csql::ExecutionStrategy> execution_strategy,
      csql::Runtime* runtime);

  void addMetric(MetricDefinition metric);

  void addDimension(DimensionDefinition dimension);

  void setFilter(String filter);

  void execute();

protected:

  RefPtr<csql::QueryPlan> buildQueryPlan();

  RefPtr<csql::QueryTreeNode> buildQueryTree(
      const MetricDefinition& metric);

  RefPtr<csql::ExecutionStrategy> execution_strategy_;
  csql::Runtime* runtime_;
  Vector<MetricDefinition> metrics_;
  Vector<DimensionDefinition> dimensions_;
  Option<String> filter_;
};

} // namespace zbase

