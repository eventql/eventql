/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRBYPAGEQUERY_H
#define _CM_CTRBYPAGEQUERY_H
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

class CTRByPageQuery : public AnalyticsSubQuery {
public:

  CTRByPageQuery(
      AnalyticsTableScan* query,
      const Vector<RefPtr<TrafficSegment>>& segments);

  RefPtr<AnalyticsQueryResult::SubQueryResult> result() override;

  size_t version() const override {
    return 2;
  }

protected:
  void onQuery();

  Vector<RefPtr<TrafficSegment>> segments_;
  RefPtr<CTRByGroupResult<uint32_t>> result_;
  RefPtr<AnalyticsTableScan::ColumnRef> time_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> page_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> clicks_col_;
};

} // namespace zbase

#endif
