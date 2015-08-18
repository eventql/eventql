/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/uri.h>
#include "CatalogCategoryDashboardQuery.h"
#include "cstable/BitPackedIntColumnReader.h"

using namespace stx;

namespace zbase {

CatalogCategoryDashboardQuery::CatalogCategoryDashboardQuery(
    AnalyticsTableScan* query,
    const Vector<RefPtr<TrafficSegment>>& segments,
    UnixTime start_time,
    UnixTime end_time,
    const String& parent_category_col,
    const String& child_category_col,
    const AnalyticsQuery::SubQueryParams& params) :
    result_(new CatalogCategoryDashboardResult()),
    segments_(segments),
    start_time_(start_time.unixMicros() / kMicrosPerSecond),
    end_time_(end_time.unixMicros() / kMicrosPerSecond),
    time_col_(query->fetchColumn("search_queries.time")),
    pcategory_col_(query->fetchColumn(parent_category_col)),
    ccategory_col_(query->fetchColumn(child_category_col)),
    pagetype_col_(query->fetchColumn("search_queries.page_type")),
    num_items_col_(query->fetchColumn("search_queries.num_result_items")),
    num_itemclicks_col_(query->fetchColumn("search_queries.num_result_items_clicked")),
    num_adimprs_col_(query->fetchColumn("search_queries.num_ad_impressions")),
    num_adclicks_col_(query->fetchColumn("search_queries.num_ad_clicks")),
    num_cartitems_col_(query->fetchColumn("search_queries.num_cart_items")),
    num_orderitems_col_(query->fetchColumn("search_queries.num_order_items")),
    cartvalue_col_(query->fetchColumn("search_queries.cart_value_eurcents")),
    gmv_col_(query->fetchColumn("search_queries.gmv_eurcents")),
    new_session_(false),
    window_secs_(kSecondsPerDay) {
  query->onQuery(std::bind(&CatalogCategoryDashboardQuery::onQuery, this));
  query->onSession(std::bind(&CatalogCategoryDashboardQuery::onSession, this));

  for (const auto& s : segments_) {
    result_->children.segment_keys.emplace_back(s->key());
    result_->children.counters.emplace_back();
    result_->parent.segment_keys.emplace_back(s->key());
    result_->parent.series.emplace_back();
  }

  parent_cat_ = 0;
  for (const auto& p : params.params) {
    if (p.first == "parent_category") {
      parent_cat_ = std::stoul(p.second);
      continue;
    }

    if (p.first == "child_category") {
      if (p.second.length() > 0) {
        child_cats_.emplace(std::stoul(p.second));
      }
    }
  }

  String window_str;
  if (URI::getParam(params.params, "window_size", &window_str)) {
    window_secs_ = std::stoull(window_str);
  }
}

void CatalogCategoryDashboardQuery::onSession() {
  new_session_ = true;
}

void CatalogCategoryDashboardQuery::onQuery() {
  auto time = time_col_->getUInt32();
  auto pagetype = (PageType) pagetype_col_->getUInt32();
  auto pcategory = pcategory_col_->getUInt32();
  auto ccategory = ccategory_col_->getUInt32();
  auto num_items = num_items_col_->getUInt32();
  auto num_clicks = num_itemclicks_col_->getUInt32();
  auto num_ad_imprs = num_adimprs_col_->getUInt32();
  auto num_ad_clicks = num_adclicks_col_->getUInt32();
  auto num_cartitems = num_cartitems_col_->getUInt32();
  auto num_orderitems = num_orderitems_col_->getUInt32();
  auto cart_value_eurcent = cartvalue_col_->getUInt32();
  auto gmv_eurcent = gmv_col_->getUInt32();

  if (pagetype != PageType::CATALOG_PAGE) {
    return;
  }

  for (int i = 0; i < segments_.size(); ++i) {
    auto pred = segments_[i]->checkPredicates();

    auto& series = result_->parent.series[i];
    auto& ts_point = series[(time / window_secs_) * window_secs_];
    if (pred && (parent_cat_ == 0 || pcategory == parent_cat_)) {
      ts_point.num_queries += 1;
      ts_point.num_queries_clicked += num_clicks > 0 ? 1 : 0;
      ts_point.num_query_items_clicked += num_clicks;
      ts_point.num_query_item_impressions += num_items;
      ts_point.num_ad_impressions += num_ad_imprs;
      ts_point.num_ads_clicked += num_ad_clicks;
      ts_point.query_num_cart_items += num_cartitems;
      ts_point.query_num_order_items += num_orderitems;
      ts_point.query_cart_value_eurcent += cart_value_eurcent;
      ts_point.query_gmv_eurcent += gmv_eurcent;
      ts_point.query_ecommerce_conversions += gmv_eurcent > 0 ? 1 : 0;
      ts_point.query_abandoned_carts += (gmv_eurcent == 0 && cart_value_eurcent > 0) ? 1 : 0;
      if (new_session_) {
        ++ts_point.num_sessions;
      }
    }

    if (child_cats_.count(ccategory) > 0) {
      auto& c_point = result_->children.counters[i][ccategory];
      if (pred) {
        c_point.num_queries += 1;
        c_point.num_queries_clicked += num_clicks > 0 ? 1 : 0;
        c_point.num_query_items_clicked += num_clicks;
        c_point.num_query_item_impressions += num_items;
        c_point.num_ad_impressions += num_ad_imprs;
        c_point.num_ads_clicked += num_ad_clicks;
        c_point.query_num_cart_items += num_cartitems;
        c_point.query_num_order_items += num_orderitems;
        c_point.query_cart_value_eurcent += cart_value_eurcent;
        c_point.query_gmv_eurcent += gmv_eurcent;
        c_point.query_ecommerce_conversions += gmv_eurcent > 0 ? 1 : 0;
        c_point.query_abandoned_carts += (gmv_eurcent == 0 && cart_value_eurcent > 0) ? 1 : 0;
        if (new_session_) {
          ++c_point.num_sessions;
        }
      }
    }
  }

  new_session_ = false;
}

RefPtr<AnalyticsQueryResult::SubQueryResult>
    CatalogCategoryDashboardQuery::result() {
  return result_.get();
}

} // namespace zbase

