/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <algorithm>
#include "CTRByResultItemCategoryQuery.h"
#include "cstable/BitPackedIntColumnReader.h"

using namespace stx;

namespace cm {

CTRByResultItemCategoryQuery::CTRByResultItemCategoryQuery(
    AnalyticsTableScan* query,
    const Vector<RefPtr<TrafficSegment>>& segments) :
    result_(new CTRByGroupResult<uint32_t>()),
    segments_(segments),
    cat1_col_(query->fetchColumn("search_queries.result_items.category1")),
    cat2_col_(query->fetchColumn("search_queries.result_items.category2")),
    cat3_col_(query->fetchColumn("search_queries.result_items.category3")),
    clicked_col_(query->fetchColumn("search_queries.result_items.clicked")) {
  query->onQueryItem(std::bind(&CTRByResultItemCategoryQuery::onQueryItem, this));

  for (const auto& s : segments_) {
    result_->segment_keys.emplace_back(s->key());
    result_->counters.emplace_back();
  }
}

void CTRByResultItemCategoryQuery::onQueryItem() {
  auto clicked = clicked_col_->getBool();
  auto cat1 = cat1_col_->getUInt32();
  auto cat2 = cat2_col_->getUInt32();
  auto cat3 = cat3_col_->getUInt32();

  for (int i = 0; i < segments_.size(); ++i) {
    if (!segments_[i]->checkPredicates()) {
      continue;
    }

    auto& counters = result_->counters[i];
    if (cat1 > 0) {
      ++counters[cat1].num_views;
      counters[cat1].num_clicks += clicked;
    }

    if (cat2 > 0) {
      ++counters[cat2].num_views;
      counters[cat2].num_clicks += clicked;
    }

    if (cat3 > 0) {
      ++counters[cat3].num_views;
      counters[cat3].num_clicks += clicked;
    }
  }
}

void CTRByResultItemCategoryQuery::reduceResult(
    RefPtr<AnalyticsQueryResult::SubQueryResult> r_) {
  auto r = dynamic_cast<CTRByGroupResult<uint32_t>*>(r_.get());

  Set<uint32_t> keep_ids;

  for (int i = 0; i < segments_.size(); ++i) {
    Vector<Pair<uint32_t, uint64_t>> segres;

    for (const auto& c : r->counters[i]) {
      segres.emplace_back(c.first, c.second.num_clicks);
    }

    std::sort(segres.begin(), segres.end(), [] (
        const Pair<uint32_t, uint64_t>& a,
        const Pair<uint32_t, uint64_t>& b) -> bool {
      return b.second < a.second;
    });

    for (size_t j = 0; j < segres.size() && j < 100; ++j) {
      keep_ids.emplace(segres[j].first);
    }
  }

  for (int i = 0; i < segments_.size(); ++i) {
    auto& counters = r->counters[i];

    for (auto c = counters.begin(); c != counters.end(); ) {
      if (keep_ids.count(c->first) == 0) {
        c = counters.erase(c);
      } else {
        ++c;
      }
    }

    for (const auto& id : keep_ids) {
      counters[id];
    }
  }
}

RefPtr<AnalyticsQueryResult::SubQueryResult> CTRByResultItemCategoryQuery::result() {
  return result_.get();
}

} // namespace cm

