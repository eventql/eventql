/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRBYPOSITIONQUERY_H
#define _CM_CTRBYPOSITIONQUERY_H
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

class CTRByPositionQuery : public AnalyticsSubQuery {
public:

  CTRByPositionQuery(
      AnalyticsTableScan* query,
      const Vector<RefPtr<TrafficSegment>>& segments);

  RefPtr<AnalyticsQueryResult::SubQueryResult> result() override;

protected:
  void onQueryItem();

  Vector<RefPtr<TrafficSegment>> segments_;
  RefPtr<CTRByGroupResult<uint16_t>> result_;
  RefPtr<AnalyticsTableScan::ColumnRef> position_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> clicked_col_;
};

} // namespace zbase

#endif
