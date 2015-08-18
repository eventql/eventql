/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include "dproc/Task.h"
#include "dproc/TaskResultFuture.h"
#include "dproc/DispatchService.h"
#include "stx/protobuf/MessageSchema.h"
#include "zbase/core/TSDBClient.h"
#include "zbase/AnalyticsQuery.h"
#include "zbase/AnalyticsQueryResult.h"

using namespace stx;

namespace cm {

class AnalyticsQueryFactory {
public:

  typedef Function<
      RefPtr<AnalyticsSubQuery> (
          const AnalyticsQuery& query,
          const AnalyticsQuery::SubQueryParams params,
          const Vector<RefPtr<TrafficSegment>>& segments,
          AnalyticsTableScan* scan)> QueryFactoryFn;

  void registerQuery(
      const String& query_type,
      QueryFactoryFn factory);

  RefPtr<AnalyticsQueryResult> buildQuery(
      const AnalyticsQuery& query,
      AnalyticsTableScan* scan);

protected:
  HashMap<String, QueryFactoryFn> query_factories_;
};

} // namespace cm

