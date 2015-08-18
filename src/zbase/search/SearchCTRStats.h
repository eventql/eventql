/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_DISCOVERYSTATS_H
#define _CM_DISCOVERYSTATS_H
#include <stx/stdtypes.h>
#include <stx/json/json.h>
#include "cstable/CSTableReader.h"
#include "zbase/CTRCounter.h"
#include "zbase/AnalyticsTableScan.h"
#include "zbase/TrafficSegment.h"
#include "zbase/AnalyticsQueryResult.h"
#include "zbase/TimeseriesResult.h"

using namespace stx;

namespace zbase {

struct SearchCTRStats {
  SearchCTRStats();

  uint64_t num_sessions;
  uint64_t num_queries;
  uint64_t num_queries_clicked;
  uint64_t num_query_items_clicked;
  uint64_t num_query_item_impressions;
  uint64_t num_ad_impressions;
  uint64_t num_ads_clicked;
  uint64_t query_num_cart_items;
  uint64_t query_num_order_items;
  uint64_t query_cart_value_eurcent;
  uint64_t query_gmv_eurcent;
  uint64_t query_ecommerce_conversions;
  uint64_t query_abandoned_carts;

  void merge(const SearchCTRStats& other);
  void toJSON(json::JSONOutputStream* json) const;
  void encode(util::BinaryMessageWriter* writer) const;
  void decode(util::BinaryMessageReader* reader);
};

} // namespace zbase

#endif
