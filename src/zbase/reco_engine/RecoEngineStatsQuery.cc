/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "RecoEngineStatsQuery.h"

#include <stx/uri.h>

using namespace stx;

namespace cm {

RecoEngineStatsQuery::RecoEngineStatsQuery(
    AnalyticsTableScan* query,
    const Vector<RefPtr<TrafficSegment>>& segments,
    UnixTime start_time,
    UnixTime end_time,
    const AnalyticsQuery::SubQueryParams& params) :
    start_time_(start_time.unixMicros() / kMicrosPerSecond),
    end_time_(end_time.unixMicros() / kMicrosPerSecond),
    result_(new TimeseriesDrilldownResult<RecoStats>()),
    segments_(segments),
    time_col_(query->fetchColumn("search_queries.time")),
    is_reco_col_(query->fetchColumn("search_queries.result_items.is_recommendation")),
    clicked_col_(query->fetchColumn("search_queries.result_items.clicked")),
    seen_col_(query->fetchColumn("search_queries.result_items.seen")),
    window_secs_(kSecondsPerDay),
    query_num_reco_items_(segments_.size(), 0),
    query_num_reco_items_clicked_(segments_.size(), 0),
    query_num_reco_items_seen_(segments_.size(), 0) {
  query->onQuery(std::bind(&RecoEngineStatsQuery::onQuery, this));
  query->onQueryItem(std::bind(&RecoEngineStatsQuery::onQueryItem, this));

  for (const auto& s : segments_) {
    result_->drilldown.segment_keys.emplace_back(s->key());
    result_->drilldown.counters.emplace_back();
    result_->timeseries.segment_keys.emplace_back(s->key());
    result_->timeseries.series.emplace_back();
  }

  String drilldown_type = "time";
  URI::getParam(params.params, "drilldown", &drilldown_type);
  setDrilldownFn(drilldown_type, query);

  String window_str;
  if (URI::getParam(params.params, "window_size", &window_str)) {
    window_secs_ = std::stoull(window_str);
  }
}

void RecoEngineStatsQuery::onQuery() {
  auto time = time_col_->getUInt32();

  auto drilldown_dim = drilldown_fn_();
  for (int i = 0; i < segments_.size(); ++i) {
    auto& series = result_->timeseries.series[i];
    auto& tpoint = series[(time / window_secs_) * window_secs_];
    auto& dpoint = result_->drilldown.counters[i][drilldown_dim];

    if (!segments_[i]->checkPredicates()) {
      continue;
    }

    tpoint.num_queries += 1;
    dpoint.num_queries += 1;
  }

  for (int i = 0; i < segments_.size(); ++i) {
    query_num_reco_items_[i] = 0;
    query_num_reco_items_clicked_[i] = 0;
    query_num_reco_items_seen_[i] = 0;
  }
}

void RecoEngineStatsQuery::onQueryItem() {
  auto time = time_col_->getUInt32();
  auto clicked = clicked_col_->getBool();
  auto is_reco = is_reco_col_->getBool();
  auto seen = clicked || seen_col_->getBool();

  if (!is_reco) {
    return;
  }

  auto drilldown_dim = drilldown_fn_();

  for (int i = 0; i < segments_.size(); ++i) {
    auto& series = result_->timeseries.series[i];
    auto& tpoint = series[(time / window_secs_) * window_secs_];
    auto& dpoint = result_->drilldown.counters[i][drilldown_dim];

    if (!segments_[i]->checkPredicates()) {
      continue;
    }

    if (++query_num_reco_items_[i] == 1) {
      ++tpoint.num_queries_with_recos;
      ++dpoint.num_queries_with_recos;
    }

    if (clicked && ++query_num_reco_items_clicked_[i] == 1) {
      ++tpoint.num_queries_with_recos_clicked;
      ++dpoint.num_queries_with_recos_clicked;
    }

    if (seen && ++query_num_reco_items_seen_[i] == 1) {
      ++tpoint.num_queries_with_recos_seen;
      ++dpoint.num_queries_with_recos_seen;
    }

    ++tpoint.num_reco_items;
    ++dpoint.num_reco_items;
    tpoint.num_reco_items_clicked += clicked > 0 ? 1 : 0;
    dpoint.num_reco_items_clicked += clicked > 0 ? 1 : 0;
    tpoint.num_reco_items_seen += seen > 0 ? 1 : 0;
    dpoint.num_reco_items_seen += seen > 0 ? 1 : 0;
  }
}

RefPtr<AnalyticsQueryResult::SubQueryResult> RecoEngineStatsQuery::result() {
  return result_.get();
}

void RecoEngineStatsQuery::setDrilldownFn(
    const String& type,
    AnalyticsTableScan* query) {
  if (type == "time") {
    drilldown_fn_ = [] { return String{}; };
  }

  else if (type == "ab_test_group") {
    auto col = query->fetchColumn("search_queries.ab_test_group");
    drilldown_fn_ = [col] {
      return StringUtil::toString(col->getUInt32());
    };
  }

  else if (type == "query_type") {
    auto col = query->fetchColumn("search_queries.query_type");
    drilldown_fn_ = [col] {
      return col->getString();
    };
  }

  else if (type == "search_experiment") {
    auto col = query->fetchColumn("search_queries.experiment");
    drilldown_fn_ = [col] () -> String {
      auto e = col->getString();
      if (e.length() > 0) {
        auto p = e.find(';');
        if (p != String::npos) {
          e.erase(e.begin() + p, e.end());
        }
        return e;
      } else {
        return "control";
      }
    };
  }

  else if (type == "session_experiment") {
    auto col = query->fetchColumn("experiment");
    drilldown_fn_ = [col] () -> String {
      auto e = col->getString();
      if (e.length() > 0) {
        auto p = e.find(';');
        if (p != String::npos) {
          e.erase(e.begin() + p, e.end());
        }
        return e;
      } else {
        return "control";
      }
    };
  }

  else {
    RAISEF(kRuntimeError, "invalid drilldown type: '$0'", type);
  }
}

} // namespace cm

