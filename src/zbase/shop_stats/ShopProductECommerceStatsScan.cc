/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "ShopProductECommerceStatsScan.h"

using namespace stx;

namespace zbase {

ShopProductECommerceStatsScan::ShopProductECommerceStatsScan(
    RefPtr<TSDBTableScanSource<ECommerceTransaction>> input,
    RefPtr<ProtoSSTableSink<ShopProductKPIs>> output,
    const ReportParams& params) :
    ReportRDD(input.get(), output.get()),
    input_(input),
    output_(output),
    params_(params) {}

void ShopProductECommerceStatsScan::onInit() {
  input_->forEach(
      std::bind(
          &ShopProductECommerceStatsScan::onTransaction,
          this,
          std::placeholders::_1));
}

void ShopProductECommerceStatsScan::onFinish() {
  for (auto& ctr : products_map_) {
    output_->addRow(ctr.first, ctr.second);
  }
}

void ShopProductECommerceStatsScan::onTransaction(const ECommerceTransaction& row) {
  for (const auto& item : row.items()) {
    auto prod = getKPIs(StringUtil::toString(item.shop_id()), item.item_id());
    if (!prod) {
      continue;
    }

    switch (row.transaction_type()) {
      case ECOMMERCE_TX_UNKNOWN:
        break;

      case ECOMMERCE_TX_PURCHASE:
        pb_incr(*prod, unique_purchases, 1);
        pb_incr(*prod, gmv_eurcent, item.price_cents()); // FIXPAUL currency conv
        break;

      case ECOMMERCE_TX_REFUND:
        pb_incr(*prod, refunded_purchases, 1);
        pb_incr(*prod, refunded_gmv_eurcent, item.price_cents()); // FIXPAUL currency conv
        break;

    }
  }
}

Option<String> ShopProductECommerceStatsScan::cacheKey() const {
  return Some(
      StringUtil::format(
          "cm.analytics.shop_stats.ShopProductECommerceStatsScan~1~$0",
          input_->cacheKey()));
}

ShopProductKPIs* ShopProductECommerceStatsScan::getKPIs(
    const String& shop_id,
    const String& product_id) {
  if (shop_id.empty()) {
    return nullptr;
  }

  auto key = shop_id + "~" + product_id;
  return &products_map_[key];
}

} // namespace zbase

