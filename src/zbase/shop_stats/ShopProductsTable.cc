/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "stx/logging.h"
#include "zbase/AnalyticsTableScanPlanner.h"
#include "ShopProductsTable.h"
#include "ShopStats.h"

using namespace stx;

namespace zbase {

ShopProductsTable::ShopProductsTable(
    const ReportParams& params,
    zbase::TSDBService* tsdb) :
    params_(params),
    tsdb_(tsdb) {
  URI::ParamList p;
  URI::parseQueryString(params.params(), &p);

  String shop_id_str;
  if (URI::getParam(p, "shop_id", &shop_id_str)) {
    setKeyPrefixFilter(shop_id_str + "~");
  }
}

Vector<String> ShopProductsTable::columns() const {
  auto cols = ShopStats::columns(ShopProductKPIs{});
  cols.insert(cols.begin(), "product_id");
  cols.insert(cols.begin() + 1, "shop_id");
  return cols;
}

void ShopProductsTable::read(dproc::TaskContext* context) {
  HashMap<String, ShopProductKPIs> map;

  AbstractProtoSSTableSource<ShopProductKPIs>::forEach([this, &map] (
      const String& key,
      const ShopProductKPIs& row) {
    ShopStats::merge(row, &map[key]);
    ShopStats::merge(row, &aggr_);
  });

  AbstractProtoSSTableSource<ShopProductKPIs>::read(context);

  for (const auto& r : map) {
    auto shop_id_end = StringUtil::find(r.first, '~');
    if (shop_id_end == String::npos) {
      RAISEF(kRuntimeError, "invalid row key: $0", r.first);
    }

    auto shop_id = r.first.substr(0, shop_id_end);
    auto product_id = r.first.substr(shop_id_end + 3);

    Vector<csql::SValue> row;
    row.emplace_back(csql::SValue::StringType(product_id));
    row.emplace_back(csql::SValue::StringType(shop_id));
    ShopStats::toRow(r.second, &row);

    if (foreach_) {
      foreach_(row);
    }
  }
}

List<dproc::TaskDependency> ShopProductsTable::dependencies() const {
  List<dproc::TaskDependency> srcs;

  srcs.splice(
      srcs.begin(),
      AnalyticsTableScanPlanner::mapShards(
          params_.customer(),
          params_.from_unixmicros(),
          params_.until_unixmicros(),
          "shop_stats.ShopProductCTRStatsScan",
          *msg::encode(params_),
          tsdb_));

  srcs.splice(
      srcs.begin(),
      AnalyticsTableScanPlanner::mapShards(
          params_.customer(),
          "events.ecommerce_transactions",
          "cm.ECommerceTransaction",
          params_.from_unixmicros(),
          params_.until_unixmicros(),
          "shop_stats.ShopProductECommerceStatsScan",
          *msg::encode(params_),
          tsdb_));

  return srcs;
}

void ShopProductsTable::forEach(
    Function<bool (const Vector<csql::SValue>&)> fn) {
  foreach_ = fn;
}

const ShopProductKPIs& ShopProductsTable::aggregates() const {
  return aggr_;
}


} // namespace zbase

