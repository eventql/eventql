/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/AnalyticsQueryFactory.h"

using namespace stx;

namespace zbase {

void AnalyticsQueryFactory::registerQuery(
    const String& query_type,
    QueryFactoryFn factory) {
  query_factories_.emplace(query_type, factory);
}

RefPtr<AnalyticsQueryResult> AnalyticsQueryFactory::buildQuery(
    const AnalyticsQuery& query,
    AnalyticsTableScan* scan) {
  auto res = mkRef(new AnalyticsQueryResult(query));

  Vector<RefPtr<zbase::TrafficSegment>> segs;
  for (const auto& s : query.segments) {
    segs.emplace_back(new TrafficSegment(s, scan));
  }

  List<RefPtr<AnalyticsSubQuery>> subqueries;
  for (const auto& subq_params : query.queries) {
    const auto& subq_type = subq_params.query_type;

    auto factory = query_factories_.find(subq_type);
    if (factory == query_factories_.end()) {
      RAISEF(kIndexError, "no query factory found for: $0", subq_type);
    }

    auto subq = factory->second(query, subq_params, segs, scan);
    subqueries.emplace_back(subq);
    res->results.emplace_back(subq->result());
    res->subqueries.emplace_back(subq);
  }

  return res;
}

} // namespace zbase


