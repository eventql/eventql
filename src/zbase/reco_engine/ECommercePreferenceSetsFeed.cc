/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "ECommercePreferenceSetsFeed.h"

using namespace stx;

namespace zbase {

ECommercePreferenceSetsFeed::ECommercePreferenceSetsFeed(
    RefPtr<TSDBTableScanSource<JoinedSession>> input,
    RefPtr<JSONSink> output) :
    ReportRDD(input.get(), output.get()),
    input_(input),
    output_(output),
    n_(0) {}

void ECommercePreferenceSetsFeed::onInit() {
  input_->forEach(
      std::bind(
          &ECommercePreferenceSetsFeed::onSession,
          this,
          std::placeholders::_1));

  output_->json()->beginArray();
}

void ECommercePreferenceSetsFeed::onSession(const JoinedSession& session) {
  auto json = output_->json();

  Set<String> visited_products;
  for (const auto& item_visit : session.page_views()) {
    visited_products.emplace(item_visit.item_id());
  }

  Set<String> bought_products;
  for (const auto& cart_item : session.cart_items()) {
    if (cart_item.checkout_step() != 1) {
      continue;
    }

    bought_products.emplace(cart_item.item_id());
  }

  if (++n_ > 1) {
    json->addComma();
  }

  json->beginObject();
  json->addObjectEntry("time");
  json->addInteger(WallClock::unixMicros() / kMicrosPerSecond);
  json->addComma();
  json->addObjectEntry("session_id");
  json->addString(session.customer_session_id());
  json->addComma();
  json->addObjectEntry("ab_test_group");
  json->addString(StringUtil::toString(session.ab_test_group()));
  json->addComma();
  json->addObjectEntry("visited_products");
  json::toJSON(visited_products, json);
  json->addComma();
  json->addObjectEntry("bought_products");
  json::toJSON(bought_products, json);
  json->endObject();
}

void ECommercePreferenceSetsFeed::onFinish() {
  output_->json()->endArray();
}

String ECommercePreferenceSetsFeed::contentType() const {
  return "application/json; charset=utf-8";
}

} // namespace zbase

