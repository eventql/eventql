/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_DISCOVERYCATALOGSTATSQUERY_H
#define _CM_DISCOVERYCATALOGSTATSQUERY_H
#include <stx/stdtypes.h>
#include <stx/json/json.h>
#include "cstable/CSTableReader.h"
#include "zbase/CTRCounter.h"
#include "zbase/AnalyticsTableScan.h"
#include "zbase/TrafficSegment.h"
#include "zbase/AnalyticsQueryResult.h"
#include "zbase/TimeseriesResult.h"
#include "zbase/search/SearchCTRStats.h"
#include "zbase/JoinedSession.pb.h"

using namespace stx;

namespace zbase {

class CatalogDashboardQuery : public AnalyticsSubQuery {
public:

  CatalogDashboardQuery(
      AnalyticsTableScan* query,
      const Vector<RefPtr<TrafficSegment>>& segments,
      UnixTime start_time,
      UnixTime end_time,
      const AnalyticsQuery::SubQueryParams& params);

  RefPtr<AnalyticsQueryResult::SubQueryResult> result() override;

  void setDrilldownFn(const String& type, AnalyticsTableScan* query);

protected:
  void onSession();
  void onQuery();

  Vector<RefPtr<TrafficSegment>> segments_;
  uint64_t start_time_;
  uint64_t end_time_;
  RefPtr<TimeseriesDrilldownResult<SearchCTRStats>> result_;
  RefPtr<AnalyticsTableScan::ColumnRef> time_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> pagetype_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> num_items_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> num_itemclicks_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> num_adimprs_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> num_adclicks_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> qnum_cartitems_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> qnum_orderitems_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> qcartvalue_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> qgmv_col_;
  bool new_session_;
  Function<String ()> drilldown_fn_;
  Function<bool ()> filter_fn_;
  uint64_t window_secs_;
};

} // namespace zbase

#endif
