/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "SearchDashboardQuery.h"

#include <stx/uri.h>
#include <stx/logging.h>

using namespace stx;

namespace zbase {

SearchDashboardQuery::SearchDashboardQuery(
    AnalyticsTableScan* query,
    const Vector<RefPtr<TrafficSegment>>& segments,
    UnixTime start_time,
    UnixTime end_time,
    const AnalyticsQuery::SubQueryParams& params) :
    start_time_(start_time.unixMicros() / kMicrosPerSecond),
    end_time_(end_time.unixMicros() / kMicrosPerSecond),
    result_(new TimeseriesDrilldownResult<SearchCTRStats>()),
    segments_(segments),
    time_col_(query->fetchColumn("search_queries.time")),
    num_items_col_(query->fetchColumn("search_queries.num_result_items")),
    num_itemclicks_col_(query->fetchColumn("search_queries.num_result_items_clicked")),
    num_adimprs_col_(query->fetchColumn("search_queries.num_ad_impressions")),
    num_adclicks_col_(query->fetchColumn("search_queries.num_ad_clicks")),
    qnum_cartitems_col_(query->fetchColumn("search_queries.num_cart_items")),
    qnum_orderitems_col_(query->fetchColumn("search_queries.num_order_items")),
    qcartvalue_col_(query->fetchColumn("search_queries.cart_value_eurcents")),
    qgmv_col_(query->fetchColumn("search_queries.gmv_eurcents")),
    new_session_(false),
    window_secs_(kSecondsPerDay),
    last_time_(0) {
  query->onSession(std::bind(&SearchDashboardQuery::onSession, this));
  query->onQuery(std::bind(&SearchDashboardQuery::onQuery, this));

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

void SearchDashboardQuery::onSession() {
  new_session_ = true;
}

void SearchDashboardQuery::onQuery() {
  auto time = time_col_->getUInt64();
  auto num_items = num_items_col_->getUInt32();
  auto num_clicks = num_itemclicks_col_->getUInt32();
  auto num_ad_imprs = num_adimprs_col_->getUInt32();
  auto num_ad_clicks = num_adclicks_col_->getUInt32();
  auto qnum_cartitems = qnum_cartitems_col_->getUInt32();
  auto qnum_orderitems = qnum_orderitems_col_->getUInt32();
  auto qcart_value_eurcent = qcartvalue_col_->getUInt32();
  auto qgmv_eurcent = qgmv_col_->getUInt32();

  if (time == 0) {
    time = last_time_;
  } else {
    last_time_ = time;
  }
  //if (pagetype != PageType::SEARCH_PAGE) {
  //  return;
  //}

  logDebug("zbase", "scan search query time: $0", time);

  auto drilldown_dim = drilldown_fn_();

  for (int i = 0; i < segments_.size(); ++i) {
    auto& tpoint = result_->timeseries.series[i][(time / window_secs_) * window_secs_];
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

RefPtr<AnalyticsQueryResult::SubQueryResult> SearchDashboardQuery::result() {
  return result_.get();
}

void SearchDashboardQuery::setDrilldownFn(
    const String& type,
    AnalyticsTableScan* query) {
  if (type == "time") {
    drilldown_fn_ = [] { return String{}; };
  }

  else if (type == "device_type") {
    auto col = query->fetchColumn("search_queries.device_type");
    drilldown_fn_ = [col] {
      return String(deviceTypeToString((DeviceType) col->getUInt32()));
    };
  }

  else if (type == "ab_test_group") {
    auto col = query->fetchColumn("search_queries.ab_test_group");
    drilldown_fn_ = [col] {
      return StringUtil::toString(col->getUInt32());
    };
  }

  else if (type == "referrer_url") {
    auto col = query->fetchColumn("referrer_url");
    drilldown_fn_ = [col] () -> String {
      auto s = col->getString();
      return s.length() == 0 ? "unknown" : s;
    };
  }

  else if (type == "referrer_name") {
    auto col = query->fetchColumn("referrer_name");
    drilldown_fn_ = [col] () -> String {
      auto s = col->getString();
      return s.length() == 0 ? "unknown" : s;
    };
  }

  else if (type == "referrer_campaign") {
    auto col = query->fetchColumn("referrer_campaign");
    drilldown_fn_ = [col] () -> String {
      auto s = col->getString();
      return s.length() == 0 ? "unknown" : s;
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

} // namespace zbase

