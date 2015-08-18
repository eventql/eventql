/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TOPSEARCHQUERIESQUERY_H
#define _CM_TOPSEARCHQUERIESQUERY_H
#include <stx/stdtypes.h>
#include <stx/json/json.h>
#include "cstable/CSTableReader.h"
#include "zbase/CTRCounter.h"
#include "zbase/AnalyticsTableScan.h"
#include "zbase/TrafficSegment.h"
#include "zbase/AnalyticsQueryResult.h"
#include "zbase/CTRByGroupResult.h"

using namespace stx;

namespace zbase {

struct TopSearchQueriesResult : public CTRByGroupResult<String> {
  void toJSON(json::JSONOutputStream* json) const;
  void toCSV(CSVOutputStream* csv) const;
  void sortResults(Vector<CTRCounter>* counters) const;

  String order_by;
  String order_fn;
  uint64_t offset;
  uint64_t limit;
  Set<String> columns;
};

class TopSearchTermsQuery : public AnalyticsSubQuery {
public:

  TopSearchTermsQuery(
      AnalyticsTableScan* query,
      const Vector<RefPtr<TrafficSegment>>& segments,
      const AnalyticsQuery::SubQueryParams& params);

  RefPtr<AnalyticsQueryResult::SubQueryResult> result() override;

  size_t version() const override {
    return 2;
  }

protected:
  void onQuery();

  Vector<RefPtr<TrafficSegment>> segments_;
  RefPtr<TopSearchQueriesResult> result_;
  RefPtr<AnalyticsTableScan::ColumnRef> time_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> page_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> qstr_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> clicks_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> gmv_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> cart_value_col_;
};

} // namespace zbase

#endif
