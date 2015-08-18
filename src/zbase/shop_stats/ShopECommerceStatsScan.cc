/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "ShopECommerceStatsScan.h"

using namespace stx;

namespace zbase {

ShopECommerceStatsScan::ShopECommerceStatsScan(
    RefPtr<TSDBTableScanSource<ECommerceTransaction>> input,
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
}

void ShopECommerceStatsScan::onInit() {
  input_->forEach(
      std::bind(
          &ShopECommerceStatsScan::onTransaction,
          this,
          std::placeholders::_1));
}

void ShopECommerceStatsScan::onFinish() {
  for (auto& ctr : shops_map_) {
    output_->addRow(ctr.first, ctr.second);
  }
}

void ShopECommerceStatsScan::onTransaction(const ECommerceTransaction& row) {
  Set<String> shop_ids;

  for (const auto& item : row.items()) {
    auto shop_id = StringUtil::toString(item.shop_id());
    auto shop = getKPIs(shop_id, row.time() * kMicrosPerSecond);
    if (!shop) {
      continue;
    }

    shop_ids.emplace(shop_id);

    switch (row.transaction_type()) {
      case ECOMMERCE_TX_UNKNOWN:
        break;

      case ECOMMERCE_TX_PURCHASE:
        pb_incr(*shop, gmv_eurcent, item.price_cents()); // FIXPAUL currency conv
        pb_incr(*shop, order_items_count, 1);
        break;

      case ECOMMERCE_TX_REFUND:
        pb_incr(*shop, refunded_gmv_eurcent, item.price_cents()); // FIXPAUL currency conv
        pb_incr(*shop, refunded_order_items_count, 1);
        break;

    }
  }

  for (const auto& shop_id : shop_ids) {
    auto shop = getKPIs(shop_id, row.time() * kMicrosPerSecond);
    if (!shop) {
      continue;
    }

    switch (row.transaction_type()) {
      case ECOMMERCE_TX_UNKNOWN:
        break;

      case ECOMMERCE_TX_PURCHASE:
        pb_incr(*shop, unique_purchases, 1);
        break;

      case ECOMMERCE_TX_REFUND:
        pb_incr(*shop, refunded_purchases, 1);
        break;

    }
  }
}

Option<String> ShopECommerceStatsScan::cacheKey() const {
  return Some(
      StringUtil::format(
          "cm.analytics.shop_stats.ShopECommerceStatsScan~6~$0~$1",
          input_->cacheKey(),
          time_window_.isEmpty() ? 0 : time_window_.get().microseconds()));
}

ShopKPIs* ShopECommerceStatsScan::getKPIs(const String& shop_id, const UnixTime& time) {
  if (shop_id.empty()) {
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

