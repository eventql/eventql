/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "DiscoveryDashboardQuery.h"

#include "cstable/BitPackedIntColumnReader.h"

using namespace stx;

namespace zbase {

DiscoveryDashboardQuery::DiscoveryDashboardQuery(
    AnalyticsTableScan* query,
    const Vector<RefPtr<TrafficSegment>>& segments,
    UnixTime start_time,
    UnixTime end_time) :
    start_time_(start_time.unixMicros() / kMicrosPerSecond),
    end_time_(end_time.unixMicros() / kMicrosPerSecond),
    result_(new TimeseriesDrilldownResult<SearchCTRStats>()),
    segments_(segments),
    time_col_(query->fetchColumn("search_queries.time")),
    pagetype_col_(query->fetchColumn("search_queries.page_type")),
    num_items_col_(query->fetchColumn("search_queries.num_result_items")),
    num_itemclicks_col_(query->fetchColumn("search_queries.num_result_items_clicked")),
    num_adimprs_col_(query->fetchColumn("search_queries.num_ad_impressions")),
    num_adclicks_col_(query->fetchColumn("search_queries.num_ad_clicks")),
    qnum_cartitems_col_(query->fetchColumn("search_queries.num_cart_items")),
    qnum_orderitems_col_(query->fetchColumn("search_queries.num_order_items")),
    qcartvalue_col_(query->fetchColumn("search_queries.cart_value_eurcents")),
    qgmv_col_(query->fetchColumn("search_queries.gmv_eurcents")),
    new_session_(false) {
  query->onSession(std::bind(&DiscoveryDashboardQuery::onSession, this));
  query->onQuery(std::bind(&DiscoveryDashboardQuery::onQuery, this));

  for (const auto& s : segments_) {
    result_->drilldown.segment_keys.emplace_back(s->key());
    result_->drilldown.counters.emplace_back();
    result_->timeseries.segment_keys.emplace_back(s->key());
    result_->timeseries.series.emplace_back();
  }
}

void DiscoveryDashboardQuery::onSession() {
  new_session_ = true;
}

void DiscoveryDashboardQuery::onQuery() {
  auto time = time_col_->getUInt32();
  auto pagetype = (PageType) pagetype_col_->getUInt32();
  auto num_items = num_items_col_->getUInt32();
  auto num_clicks = num_itemclicks_col_->getUInt32();
  auto num_ad_imprs = num_adimprs_col_->getUInt32();
  auto num_ad_clicks = num_adclicks_col_->getUInt32();
  auto qnum_cartitems = qnum_cartitems_col_->getUInt32();
  auto qnum_orderitems = qnum_orderitems_col_->getUInt32();
  auto qcart_value_eurcent = qcartvalue_col_->getUInt32();
  auto qgmv_eurcent = qgmv_col_->getUInt32();

  //if (time < start_time_ || time >= end_time_) {
  //  return;
  //}

  //if (pagetype != PageType::CATALOG_PAGE && pagetype != PageType::SEARCH_PAGE) {
  //  return;
  //}

  auto drilldown_dim = pageTypeToString(pagetype);

  for (int i = 0; i < segments_.size(); ++i) {
    auto& series = result_->timeseries.series[i];
    auto& tpoint = series[(time / kSecondsPerDay) * kSecondsPerDay];
    auto& dpoint = result_->drilldown.counters[i][drilldown_dim];

    if (!segments_[i]->checkPredicates()) {
      continue;
    }

    tpoint.num_queries += 1;
    dpoint.num_queries += 1;
    tpoint.num_queries_clicked += num_clicks > 0 ? 1 : 0;
    dpoint.num_queries_clicked += num_clicks > 0 ? 1 : 0;
    tpoint.num_query_items_clicked += num_clicks;
    dpoint.num_query_items_clicked += num_clicks;
    tpoint.num_query_item_impressions += num_items;
    dpoint.num_query_item_impressions += num_items;
    tpoint.num_ad_impressions += num_ad_imprs;
    dpoint.num_ad_impressions += num_ad_imprs;
    tpoint.num_ads_clicked += num_ad_clicks;
    dpoint.num_ads_clicked += num_ad_clicks;
    tpoint.query_num_cart_items += qnum_cartitems;
    dpoint.query_num_cart_items += qnum_cartitems;
    tpoint.query_num_order_items += qnum_orderitems;
    dpoint.query_num_order_items += qnum_orderitems;
    tpoint.query_cart_value_eurcent += qcart_value_eurcent;
    dpoint.query_cart_value_eurcent += qcart_value_eurcent;
    tpoint.query_gmv_eurcent += qgmv_eurcent;
    dpoint.query_gmv_eurcent += qgmv_eurcent;
    tpoint.query_ecommerce_conversions += qgmv_eurcent > 0 ? 1 : 0;
    dpoint.query_ecommerce_conversions += qgmv_eurcent > 0 ? 1 : 0;
    tpoint.query_abandoned_carts += (qgmv_eurcent == 0 && qcart_value_eurcent > 0) ? 1 : 0;
    dpoint.query_abandoned_carts += (qgmv_eurcent == 0 && qcart_value_eurcent > 0) ? 1 : 0;

    if (new_session_) {
      ++tpoint.num_sessions;
      ++dpoint.num_sessions;
    }
  }

  new_session_ = false;
}

RefPtr<AnalyticsQueryResult::SubQueryResult> DiscoveryDashboardQuery::result() {
  return result_.get();
}

} // namespace zbase

