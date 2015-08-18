/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "CTRByPositionQuery.h"
#include "cstable/BitPackedIntColumnReader.h"

using namespace stx;

namespace zbase {

CTRByPositionQuery::CTRByPositionQuery(
    AnalyticsTableScan* query,
    const Vector<RefPtr<TrafficSegment>>& segments) :
    result_(new CTRByGroupResult<uint16_t>()),
    segments_(segments),
    position_col_(query->fetchColumn("search_queries.result_items.position")),
    clicked_col_(query->fetchColumn("search_queries.result_items.clicked")) {
  query->onQueryItem(std::bind(&CTRByPositionQuery::onQueryItem, this));

  for (const auto& s : segments_) {
    result_->segment_keys.emplace_back(s->key());
    result_->counters.emplace_back();
  }
}

void CTRByPositionQuery::onQueryItem() {
  auto pos = position_col_->getUInt32();
  if (pos < 1 || pos > 44) {
    return;
  }

  auto clicked = clicked_col_->getBool();

  for (int i = 0; i < segments_.size(); ++i) {
    if (!segments_[i]->checkPredicates()) {
      continue;
    }

    auto& counters = result_->counters[i];
    ++counters[pos].num_views;
    counters[pos].num_clicks += clicked;
  }
}

RefPtr<AnalyticsQueryResult::SubQueryResult> CTRByPositionQuery::result() {
  return result_.get();
}

} // namespace zbase

