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
#include "ShopKPITable.h"
#include "ShopStats.h"

using namespace stx;

namespace cm {

ShopKPITable::ShopKPITable(
    const ReportParams& params,
    tsdb::TSDBService* tsdb) :
    params_(params),
    tsdb_(tsdb) {
  URI::ParamList p;
  URI::parseQueryString(params.params(), &p);

  String time_window_str;
  if (URI::getParam(p, "time_window", &time_window_str)) {
    Duration time_window(std::stoull(time_window_str) * kMicrosPerSecond);
    time_window_ = Some(time_window);
  }

  String shop_id_str;
  if (URI::getParam(p, "shop_id", &shop_id_str)) {
    setKeyPrefixFilter(shop_id_str + "~");
  }
}

Vector<String> ShopKPITable::columns() const {
  auto cols = ShopStats::columns(ShopKPIs{});
  cols.insert(cols.begin(), "shop_id");
  if (!time_window_.isEmpty()) {
    cols.insert(cols.begin() + 1, "time");
  }

  return cols;
}

void ShopKPITable::read(dproc::TaskContext* context) {
  HashMap<String, ShopKPIs> map;

  AbstractProtoSSTableSource<ShopKPIs>::forEach([this, &map] (
      const String& key,
      const ShopKPIs& row) {
    ShopStats::merge(row, &map[key]);
    ShopStats::merge(row, &aggr_);
  });

  AbstractProtoSSTableSource<ShopKPIs>::read(context);

  for (const auto& r : map) {
    const auto& key = r.first;
    const auto& shop = r.second;

    auto shop_id_end = StringUtil::find(key, '~');
    auto shop_id = shop_id_end == String::npos ? key : key.substr(0, shop_id_end);

    auto time = 0;
    if (shop_id_end != String::npos) {
      auto time_str = key.substr(shop_id_end + 1);
      if (!time_str.empty()) {
        time = std::stoull(time_str);
      }
    }

    Vector<csql::SValue> row;
    row.emplace_back(csql::SValue::StringType(shop_id));

    if (!time_window_.isEmpty()) {
      row.emplace_back(csql::SValue::TimeType(time * kMicrosPerSecond));
    }

    ShopStats::toRow(shop, &row);
    if (foreach_) {
      foreach_(row);
    }
  }
}

List<dproc::TaskDependency> ShopKPITable::dependencies() const {
  List<dproc::TaskDependency> srcs;

  srcs.splice(
      srcs.begin(),
      AnalyticsTableScanPlanner::mapShards(
          params_.customer(),
          params_.from_unixmicros(),
          params_.until_unixmicros(),
          "shop_stats.ShopCTRStatsScan",
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
          "shop_stats.ShopECommerceStatsScan",
          *msg::encode(params_),
          tsdb_));

  return srcs;
}

void ShopKPITable::forEach(
    Function<bool (const Vector<csql::SValue>&)> fn) {
  foreach_ = fn;
}

const ShopKPIs& ShopKPITable::aggregates() const {
  return aggr_;
}

} // namespace cm

