/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "CTRByPageQuery.h"
#include "cstable/BitPackedIntColumnReader.h"

using namespace stx;

namespace zbase {

CTRByPageQuery::CTRByPageQuery(
    AnalyticsTableScan* query,
    const Vector<RefPtr<TrafficSegment>>& segments) :
    result_(new CTRByGroupResult<uint32_t>()),
    segments_(segments),
    time_col_(query->fetchColumn("search_queries.time")),
    page_col_(query->fetchColumn("search_queries.page")),
    clicks_col_(query->fetchColumn("search_queries.num_result_items_clicked")) {
  query->onQuery(std::bind(&CTRByPageQuery::onQuery, this));

  for (const auto& s : segments_) {
    result_->segment_keys.emplace_back(s->key());
    result_->counters.emplace_back();
  }
}

void CTRByPageQuery::onQuery() {
  auto page = page_col_->getUInt32();
  auto clicks = clicks_col_->getUInt32();
  if (page < 1 || page > 100) {
    return;
  }

  for (int i = 0; i < segments_.size(); ++i) {
    if (!segments_[i]->checkPredicates()) {
      continue;
    }

    auto& counter = result_->counters[i][page];
    ++counter.num_views;
    counter.num_clicks += clicks > 0 ? 1 : 0;
    counter.num_clicked += clicks;
  }
}

RefPtr<AnalyticsQueryResult::SubQueryResult> CTRByPageQuery::result() {
  return result_.get();
}

} // namespace zbase

