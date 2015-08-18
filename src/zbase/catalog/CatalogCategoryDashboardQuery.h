/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_DISCOVERYCATEGORYSTATSQUERY_H
#define _CM_DISCOVERYCATEGORYSTATSQUERY_H
#include <stx/stdtypes.h>
#include <stx/json/json.h>
#include "cstable/CSTableReader.h"
#include "zbase/CTRCounter.h"
#include "zbase/AnalyticsTableScan.h"
#include "zbase/TrafficSegment.h"
#include "zbase/AnalyticsQueryResult.h"
#include "zbase/catalog/CatalogCategoryDashboardResult.h"


using namespace stx;

namespace zbase {

class CatalogCategoryDashboardQuery : public AnalyticsSubQuery {
public:

  CatalogCategoryDashboardQuery(
      AnalyticsTableScan* query,
      const Vector<RefPtr<TrafficSegment>>& segments,
      UnixTime start_time,
      UnixTime end_time,
      const String& parent_category_col,
      const String& child_category_col,
      const AnalyticsQuery::SubQueryParams& params);

  RefPtr<AnalyticsQueryResult::SubQueryResult> result() override;

protected:
  void onQuery();
  void onSession();

  Vector<RefPtr<TrafficSegment>> segments_;
  uint64_t start_time_;
  uint64_t end_time_;
  RefPtr<CatalogCategoryDashboardResult> result_;
  RefPtr<AnalyticsTableScan::ColumnRef> pagetype_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> pcategory_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> ccategory_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> time_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> num_items_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> num_itemclicks_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> num_adimprs_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> num_adclicks_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> num_cartitems_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> num_orderitems_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> gmv_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> cartvalue_col_;
  uint32_t parent_cat_;
  Set<uint32_t> child_cats_;
  bool new_session_;
  uint64_t window_secs_;
};

} // namespace zbase

#endif
