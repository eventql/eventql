/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_DISCOVERYCATEGORYSTATSRESULT_H
#define _CM_DISCOVERYCATEGORYSTATSRESULT_H
#include <stx/stdtypes.h>
#include <stx/json/json.h>
#include "cstable/CSTableReader.h"
#include "zbase/CTRCounter.h"
#include "zbase/AnalyticsTableScan.h"
#include "zbase/TrafficSegment.h"
#include "zbase/AnalyticsQueryResult.h"
#include "zbase/TimeseriesResult.h"
#include "zbase/GroupResult.h"
#include "zbase/search/SearchCTRStats.h"

using namespace stx;

namespace zbase {

struct CatalogCategoryDashboardResult :
    public AnalyticsQueryResult::SubQueryResult {
  void merge(const AnalyticsQueryResult::SubQueryResult& other);
  void toJSON(json::JSONOutputStream* json) const override;
  void encode(util::BinaryMessageWriter* writer) const override;
  void decode(util::BinaryMessageReader* reader) override;
  void applyTimeRange(UnixTime from, UnixTime until) override;

  TimeseriesResult<SearchCTRStats> parent;
  GroupResult<uint32_t, SearchCTRStats> children;
};

} // namespace zbase

#endif
