/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ECOMMERCEKPIQUERY_H
#define _CM_ECOMMERCEKPIQUERY_H
#include <stx/stdtypes.h>
#include <stx/json/json.h>
#include "cstable/CSTableReader.h"
#include "zbase/CTRCounter.h"
#include "zbase/AnalyticsTableScan.h"
#include "zbase/TrafficSegment.h"
#include "zbase/AnalyticsQueryResult.h"
#include "zbase/TimeseriesResult.h"

using namespace stx;

namespace cm {

struct ECommerceKPIs {
  ECommerceKPIs();

  uint64_t num_sessions;
  uint64_t num_cart_items;
  uint64_t num_order_items;
  uint64_t num_purchases;
  uint64_t num_carts;
  uint64_t cart_value_eurcent;
  uint64_t gmv_eurcent;
  uint64_t abandoned_carts;

  void merge(const ECommerceKPIs& other);
  void toJSON(json::JSONOutputStream* json) const;
  void encode(util::BinaryMessageWriter* writer) const;
  void decode(util::BinaryMessageReader* reader);
};

class ECommerceKPIQuery : public AnalyticsSubQuery {
public:

  ECommerceKPIQuery(
      AnalyticsTableScan* query,
      const Vector<RefPtr<TrafficSegment>>& segments,
      UnixTime start_time,
      UnixTime end_time,
      const AnalyticsQuery::SubQueryParams& params);

  RefPtr<AnalyticsQueryResult::SubQueryResult> result() override;

  void setDrilldownFn(const String& type, AnalyticsTableScan* query);

protected:
  void onSession();
  void onCartItem();

  Vector<RefPtr<TrafficSegment>> segments_;
  uint64_t start_time_;
  uint64_t end_time_;
  RefPtr<TimeseriesDrilldownResult<ECommerceKPIs>> result_;
  RefPtr<AnalyticsTableScan::ColumnRef> time_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> price_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> quantity_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> step_col_;
  Function<String ()> drilldown_fn_;
  Vector<uint32_t> cur_cart_size_;
  Vector<uint32_t> cur_order_size_;
  Vector<uint32_t> num_sessions_carry_;
  uint64_t window_secs_;
};

} // namespace cm

#endif
