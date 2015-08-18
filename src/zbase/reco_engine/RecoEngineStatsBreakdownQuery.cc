/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "RecoEngineStatsBreakdownQuery.h"

#include <stx/uri.h>

using namespace stx;

namespace zbase {

RecoEngineStatsBreakdownQuery::RecoEngineStatsBreakdownQuery(
    AnalyticsTableScan* query,
    const Vector<RefPtr<TrafficSegment>>& segments,
    UnixTime start_time,
    UnixTime end_time,
    const AnalyticsQuery::SubQueryParams& params) :
    start_time_(start_time.unixMicros() / kMicrosPerSecond),
    end_time_(end_time.unixMicros() / kMicrosPerSecond),
    result_(new TimeseriesBreakdownResult<RecoStats>()),
    segments_(segments),
    time_col_(query->fetchColumn("search_queries.time")),
    is_reco_col_(query->fetchColumn("search_queries.result_items.is_recommendation")),
    clicked_col_(query->fetchColumn("search_queries.result_items.clicked")),
    seen_col_(query->fetchColumn("search_queries.result_items.seen")),
    window_secs_(kSecondsPerDay),
    query_num_reco_items_(0),
    query_num_reco_items_clicked_(0),
    query_num_reco_items_seen_(0) {
  query->onQuery(std::bind(&RecoEngineStatsBreakdownQuery::onQuery, this));
  query->onQueryItem(std::bind(&RecoEngineStatsBreakdownQuery::onQueryItem, this));

  String window_str;
  if (URI::getParam(params.params, "window_size", &window_str)) {
    window_secs_ = std::stoull(window_str);
  }

  String dims_str;
  if (URI::getParam(params.params, "dimensions", &dims_str)) {
    result_->dimensions = StringUtil::split(dims_str, ",");

    for (const auto& d : result_->dimensions) {
      dimensions_.emplace_back(getDimension(d, query));
    }
  }
}

void RecoEngineStatsBreakdownQuery::onQuery() {
  auto time = time_col_->getUInt32();

  auto dim_key = dimensionKey();
  auto& point = result_->timeseries[(time / window_secs_) * window_secs_][dim_key];

  point.num_queries += 1;
  point.num_queries += 1;

  query_num_reco_items_ = 0;
  query_num_reco_items_clicked_ = 0;
  query_num_reco_items_seen_ = 0;
}

void RecoEngineStatsBreakdownQuery::onQueryItem() {
  auto time = time_col_->getUInt32();
  auto clicked = clicked_col_->getBool();
  auto is_reco = is_reco_col_->getBool();
  auto seen = clicked || seen_col_->getBool();

  if (!is_reco) {
    return;
  }

  auto dim_key = dimensionKey();
  auto& point = result_->timeseries[(time / window_secs_) * window_secs_][dim_key];

  if (++query_num_reco_items_ == 1) {
    ++point.num_queries_with_recos;
  }

  if (clicked && ++query_num_reco_items_clicked_== 1) {
    ++point.num_queries_with_recos_clicked;
  }

  if (seen && ++query_num_reco_items_seen_ == 1) {
    ++point.num_queries_with_recos_seen;
  }

  ++point.num_reco_items;
  point.num_reco_items_clicked += clicked > 0 ? 1 : 0;
  point.num_reco_items_seen += seen > 0 ? 1 : 0;
}

RefPtr<AnalyticsQueryResult::SubQueryResult>
    RecoEngineStatsBreakdownQuery::result() {
  return result_.get();
}

String RecoEngineStatsBreakdownQuery::dimensionKey() {
  Vector<String> dim_values;
  for (const auto& d : dimensions_) {
    dim_values.emplace_back(d());
  }

  return StringUtil::join(dim_values, "~");
}

Function<String ()> RecoEngineStatsBreakdownQuery::getDimension(
    const String& dimension,
    AnalyticsTableScan* query) {
  if (dimension == "ab_test_group") {
    auto col = query->fetchColumn("search_queries.ab_test_group");
    return [col] {
      return StringUtil::toString(col->getUInt32());
    };
  }

  else if (dimension == "query_type") {
    auto col = query->fetchColumn("search_queries.query_type");
    return [col] {
      return col->getString();
    };
  }

  else {
    RAISEF(kRuntimeError, "dimension not defined: '$0'", dimension);
  }
}

} // namespace zbase

