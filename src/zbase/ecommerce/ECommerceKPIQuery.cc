/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "ECommerceKPIQuery.h"

#include <stx/uri.h>

using namespace stx;

namespace cm {

ECommerceKPIQuery::ECommerceKPIQuery(
    AnalyticsTableScan* query,
    const Vector<RefPtr<TrafficSegment>>& segments,
    UnixTime start_time,
    UnixTime end_time,
    const AnalyticsQuery::SubQueryParams& params) :
    start_time_(start_time.unixMicros() / kMicrosPerSecond),
    end_time_(end_time.unixMicros() / kMicrosPerSecond),
    result_(new TimeseriesDrilldownResult<ECommerceKPIs>()),
    segments_(segments),
    time_col_(query->fetchColumn("cart_items.time")),
    price_col_(query->fetchColumn("cart_items.price_cents")),
    quantity_col_(query->fetchColumn("cart_items.quantity")),
    step_col_(query->fetchColumn("cart_items.checkout_step")),
    window_secs_(kSecondsPerDay) {
  query->onSession(std::bind(&ECommerceKPIQuery::onSession, this));
  query->onCartItem(std::bind(&ECommerceKPIQuery::onCartItem, this));

  for (const auto& s : segments_) {
    result_->timeseries.segment_keys.emplace_back(s->key());
    result_->timeseries.series.emplace_back();
    result_->drilldown.segment_keys.emplace_back(s->key());
    result_->drilldown.counters.emplace_back();
    cur_order_size_.emplace_back(0);
    cur_cart_size_.emplace_back(0);
    num_sessions_carry_.emplace_back(0);
  }

  String drilldown_type = "time";
  URI::getParam(params.params, "drilldown", &drilldown_type);
  setDrilldownFn(drilldown_type, query);

  String window_str;
  if (URI::getParam(params.params, "window_size", &window_str)) {
    window_secs_ = std::stoull(window_str);
  }
}

void ECommerceKPIQuery::onSession() {
  cur_order_size_.clear();
  cur_cart_size_.clear();
  for (int i = 0; i < segments_.size(); ++i) {
    cur_order_size_.emplace_back(0);
    cur_cart_size_.emplace_back(0);
    ++num_sessions_carry_[i];
  }
}

void ECommerceKPIQuery::onCartItem() {
  auto time = time_col_->getUInt64() / kMicrosPerSecond;
  auto price = price_col_->getUInt32();
  auto step = step_col_->getUInt32();
  auto quantity = quantity_col_->getUInt32();
  auto gross_value = price * quantity;

  auto drilldown_dim = drilldown_fn_();

  for (int i = 0; i < segments_.size(); ++i) {
    auto& series = result_->timeseries.series[i];
    auto& tpoint = series[(time / window_secs_) * window_secs_];
    auto& dpoint = result_->drilldown.counters[i][drilldown_dim];

    if (!segments_[i]->checkPredicates()) {
      continue;
    }

    auto was_bought = step == 1;
    tpoint.num_sessions += num_sessions_carry_[i];
    dpoint.num_sessions += num_sessions_carry_[i];
    ++tpoint.num_cart_items;
    ++dpoint.num_cart_items;
    tpoint.cart_value_eurcent += gross_value;
    dpoint.cart_value_eurcent += gross_value;

    if (++cur_cart_size_[i] == 1) {
      ++tpoint.num_carts;
      ++dpoint.num_carts;
    }

    if (was_bought) {
      ++tpoint.num_order_items;
      ++dpoint.num_order_items;

      tpoint.gmv_eurcent += gross_value;
      dpoint.gmv_eurcent += gross_value;
      if (++cur_order_size_[i] == 1) {
        ++tpoint.num_purchases;
        ++dpoint.num_purchases;
      }
    }
  }

  for (int i = 0; i < segments_.size(); ++i) {
    num_sessions_carry_[i] = 0;
  }
}

RefPtr<AnalyticsQueryResult::SubQueryResult> ECommerceKPIQuery::result() {
  return result_.get();
}

ECommerceKPIs::ECommerceKPIs() :
    num_sessions(0),
    num_cart_items(0),
    num_order_items(0),
    num_purchases(0),
    num_carts(0),
    cart_value_eurcent(0),
    gmv_eurcent(0),
    abandoned_carts(0) {}

void ECommerceKPIs::merge(const ECommerceKPIs& other) {
  num_sessions += other.num_sessions;
  num_cart_items += other.num_cart_items;
  num_order_items += other.num_order_items;
  num_purchases += other.num_purchases;
  num_carts += other.num_carts;
  cart_value_eurcent += other.cart_value_eurcent;
  gmv_eurcent += other.gmv_eurcent;
  abandoned_carts += other.abandoned_carts;
}

void ECommerceKPIs::encode(util::BinaryMessageWriter* writer) const {
  writer->appendUInt8(0x02);
  writer->appendVarUInt(num_sessions);
  writer->appendVarUInt(num_cart_items);
  writer->appendVarUInt(num_order_items);
  writer->appendVarUInt(num_purchases);
  writer->appendVarUInt(num_carts);
  writer->appendVarUInt(cart_value_eurcent);
  writer->appendVarUInt(gmv_eurcent);
  writer->appendVarUInt(abandoned_carts);
}

void ECommerceKPIs::decode(util::BinaryMessageReader* reader) {
  if (*reader->readUInt8() != 0x02) {
    RAISE(kRuntimeError, "unsupported version");
  }

  num_sessions = reader->readVarUInt();
  num_cart_items = reader->readVarUInt();
  num_order_items = reader->readVarUInt();
  num_purchases = reader->readVarUInt();
  num_carts = reader->readVarUInt();
  cart_value_eurcent = reader->readVarUInt();
  gmv_eurcent = reader->readVarUInt();
  abandoned_carts = reader->readVarUInt();
}

static double mkRate(double num, double div) {
  if (div == 0 || num == 0) {
    return 0;
  }

  return num / div;
}

void ECommerceKPIs::toJSON(json::JSONOutputStream* json) const {
  json->beginObject();

  json->addObjectEntry("num_sessions");
  json->addInteger(num_sessions);
  json->addComma();

  json->addObjectEntry("num_cart_items");
  json->addInteger(num_cart_items);
  json->addComma();

  json->addObjectEntry("num_order_items");
  json->addInteger(num_order_items);
  json->addComma();

  json->addObjectEntry("num_purchases");
  json->addInteger(num_purchases);
  json->addComma();

  json->addObjectEntry("num_carts");
  json->addInteger(num_carts);
  json->addComma();

  json->addObjectEntry("ecommerce_conversion_rate");
  json->addFloat(mkRate(num_purchases, num_sessions));
  json->addComma();

  json->addObjectEntry("total_cart_value_eurcent");
  json->addInteger(cart_value_eurcent);
  json->addComma();

  json->addObjectEntry("cart_value_eurcent_per_session");
  json->addFloat(mkRate(cart_value_eurcent, num_sessions));
  json->addComma();

  json->addObjectEntry("total_gmv_eurcent");
  json->addInteger(gmv_eurcent);
  json->addComma();

  json->addObjectEntry("gmv_eurcent_per_session");
  json->addFloat(mkRate(gmv_eurcent, num_sessions));
  json->addComma();

  json->addObjectEntry("gmv_eurcent_per_transaction");
  json->addFloat(mkRate(gmv_eurcent, num_purchases));
  json->addComma();

  json->addObjectEntry("num_abandoned_carts");
  json->addInteger(num_carts - num_purchases);
  json->addComma();

  json->addObjectEntry("abandoned_cart_rate");
  json->addFloat(1.0 - mkRate(num_purchases, num_carts));

  json->endObject();
}

void ECommerceKPIQuery::setDrilldownFn(
    const String& type,
    AnalyticsTableScan* query) {
  if (type == "time") {
    drilldown_fn_ = [] { return String{}; };
  }

  else if (type == "ab_test_group") {
    auto col = query->fetchColumn("ab_test_group");
    drilldown_fn_ = [col] { return StringUtil::toString(col->getUInt32()); };
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

