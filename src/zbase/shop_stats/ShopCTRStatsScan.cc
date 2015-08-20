/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "ShopCTRStatsScan.h"

using namespace stx;

namespace zbase {

ShopCTRStatsScan::ShopCTRStatsScan(
    RefPtr<TSDBTableScanSource<NewJoinedSession>> input,
    RefPtr<ProtoSSTableSink<ShopKPIs>> output,
    const ReportParams& params) :
    ReportRDD(input.get(), output.get()),
    input_(input),
    output_(output),
    params_(params) {
  URI::ParamList p;
  URI::parseQueryString(params.params(), &p);

  String time_window_str;
  if (URI::getParam(p, "time_window", &time_window_str)) {
    Duration time_window(std::stoull(time_window_str) * kMicrosPerSecond);
    time_window_ = Some(time_window);
  }

  input_->setRequiredFields(
      Set<String> {
        "event.page_view.time",
        "event.page_view.item_id",
        "event.page_view.shop_id",
        "event.search_query.time",
        "event.search_query.page_type",
        "event.search_query.result_items.item_id",
        "event.search_query.result_items.position",
        "event.search_query.result_items.clicked",
        "event.search_query.result_items.shop_id",
        "event.search_query.shop_id",
        "event.cart_items.time",
        "event.cart_items.item_id",
        "event.cart_items.shop_id",
        "event.cart_items.price_cents",
        "event.cart_items.quantity",
        "event.cart_items.currency",
        "event.cart_items.checkout_step"
      });
}

void ShopCTRStatsScan::onInit() {
  input_->forEach(
      std::bind(
          &ShopCTRStatsScan::onSession,
          this,
          std::placeholders::_1));
}

void ShopCTRStatsScan::onFinish() {
  for (auto& ctr : shops_map_) {
    output_->addRow(ctr.first, ctr.second);
  }
}

void ShopCTRStatsScan::onSession(const NewJoinedSession& row) {
  for (const auto& sq : row.event().search_query()) {
    Set<ShopKPIs*> all_shops;
    Set<ShopKPIs*> all_clicked_shops;

    // N.B. this is a hack that we had to put in b/c shop page detection wasn't
    // implemented in the logjoin  for the first runs. remove as soon as that
    // works and a full session reindex was performed
    auto page_type = sq.page_type();
    if (sq.shop_id() > 0) {
      page_type = PAGETYPE_SHOP_PAGE;
    }

    for (const auto& ri : sq.result_items()) {
      auto shop = getKPIs(
          StringUtil::toString(ri.shop_id()),
          sq.time() * kMicrosPerSecond);

      if (!shop) {
        continue;
      }

      all_shops.emplace(shop);
      if (ri.clicked()) {
        all_clicked_shops.emplace(shop);
      }

      if (ri.is_paid_result()) {
        pb_incr(*shop, ad_impressions, 1);
        if (ri.clicked()) pb_incr(*shop, ad_clicks, 1);
      }

      switch (page_type) {

        case PAGETYPE_CATALOG_PAGE:
          pb_incr(*shop, catalog_listview_impressions, 1);
          if (ri.clicked()) pb_incr(*shop, catalog_listview_clicks, 1);
          break;

        case PAGETYPE_SEARCH_PAGE:
          pb_incr(*shop, search_listview_impressions, 1);
          if (ri.clicked()) pb_incr(*shop, search_listview_clicks, 1);
          break;

        case PAGETYPE_SHOP_PAGE:
          pb_incr(*shop, shop_page_listview_impressions, 1);
          if (ri.clicked()) pb_incr(*shop, shop_page_listview_clicks, 1);
          break;

        default:
          break;

      }
    }

    for (auto shop : all_shops) {
      switch (page_type) {

        case PAGETYPE_CATALOG_PAGE:
          pb_incr(*shop, catalog_impressions, 1);
          break;

        case PAGETYPE_SEARCH_PAGE:
          pb_incr(*shop, search_impressions, 1);
          break;

        case PAGETYPE_SHOP_PAGE:
          pb_incr(*shop, shop_page_impressions, 1);
          break;

        default:
          break;

      }
    }

    for (auto shop : all_clicked_shops) {
      switch (page_type) {

        case PAGETYPE_CATALOG_PAGE:
          pb_incr(*shop, catalog_clicks, 1);
          break;

        case PAGETYPE_SEARCH_PAGE:
          pb_incr(*shop, search_clicks, 1);
          break;

        case PAGETYPE_SHOP_PAGE:
          pb_incr(*shop, shop_page_clicks, 1);
          break;

        default:
          break;

      }
    }

  }

  for (const auto& ci : row.event().cart_items()) {
    auto shop = getKPIs(
        StringUtil::toString(ci.shop_id()),
        ci.time() * kMicrosPerSecond);

    if (!shop) {
      continue;
    }

    pb_incr(*shop, cart_items_count, 1);
  }

  for (const auto& iv : row.event().page_view()) {
    auto shop = getKPIs(StringUtil::toString(iv.shop_id()), iv.time() * kMicrosPerSecond);
    if (!shop) {
      continue;
    }

    pb_incr(*shop, product_page_impressions, 1);
  }
}

Option<String> ShopCTRStatsScan::cacheKey() const {
  return Some(
      StringUtil::format(
          "cm.analytics.shop_stats.ShopCTRStatsScan~8~$0~$1",
          input_->cacheKey(),
          time_window_.isEmpty() ? 0 : time_window_.get().microseconds()));
}

ShopKPIs* ShopCTRStatsScan::getKPIs(const String& shop_id, const UnixTime& time) {
  if (shop_id.empty() || shop_id == "0") {
    return nullptr;
  }

  auto key = shop_id + "~";
  if (!time_window_.isEmpty()) {
    auto w = time_window_.get().microseconds();
    auto twin = (time.unixMicros() / w) * w / kMicrosPerSecond;
    key += StringUtil::format("$0", twin);
  }

  return &shops_map_[key];
}


} // namespace zbase

