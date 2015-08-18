/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_RECOENGINESTATSQUERY_H
#define _CM_RECOENGINESTATSQUERY_H
#include <stx/stdtypes.h>
#include <stx/json/json.h>
#include "cstable/CSTableReader.h"
#include "zbase/CTRCounter.h"
#include "zbase/AnalyticsTableScan.h"
#include "zbase/TrafficSegment.h"
#include "zbase/AnalyticsQueryResult.h"
#include "zbase/TimeseriesResult.h"
#include "zbase/reco_engine/RecoStats.h"

using namespace stx;

namespace cm {

class RecoEngineStatsQuery : public AnalyticsSubQuery {
public:

  RecoEngineStatsQuery(
      AnalyticsTableScan* query,
      const Vector<RefPtr<TrafficSegment>>& segments,
      UnixTime start_time,
      UnixTime end_time,
      const AnalyticsQuery::SubQueryParams& params);

  RefPtr<AnalyticsQueryResult::SubQueryResult> result() override;

  void setDrilldownFn(const String& type, AnalyticsTableScan* query);

  size_t version() const override {
    return 7;
  }

protected:
  void onQuery();
  void onQueryItem();

  Vector<RefPtr<TrafficSegment>> segments_;
  uint64_t start_time_;
  uint64_t end_time_;
  RefPtr<TimeseriesDrilldownResult<RecoStats>> result_;
  RefPtr<AnalyticsTableScan::ColumnRef> time_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> is_reco_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> clicked_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> seen_col_;
  Function<String ()> drilldown_fn_;
  uint64_t window_secs_;
  Vector<size_t> query_num_reco_items_;
  Vector<size_t> query_num_reco_items_seen_;
  Vector<size_t> query_num_reco_items_clicked_;
};

} // namespace cm

#endif
