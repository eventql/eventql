/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "ECommerceSearchQueriesFeed.h"

using namespace stx;

namespace cm {

ECommerceSearchQueriesFeed::ECommerceSearchQueriesFeed(
    RefPtr<TSDBTableScanSource<JoinedSession>> input,
    RefPtr<JSONSink> output) :
    ReportRDD(input.get(), output.get()),
    input_(input),
    output_(output),
    n_(0) {}

void ECommerceSearchQueriesFeed::onInit() {
  input_->forEach(
      std::bind(
          &ECommerceSearchQueriesFeed::onSession,
          this,
          std::placeholders::_1));

  output_->json()->beginArray();
}

void ECommerceSearchQueriesFeed::onSession(const JoinedSession& session) {
  auto json = output_->json();

  for (const auto& q : session.search_queries()) {
    Set<String> product_list;
    Set<String> clicked_products;
    for (const auto& item : q.result_items()) {
      product_list.emplace(item.item_id());
      if (item.clicked()) {
        clicked_products.emplace(item.item_id());
      }
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
    json->addObjectEntry("query_string");
    json->addString(q.query_string_normalized());
    json->addComma();
    json->addObjectEntry("page_number");
    json->addInteger(q.page());
    json->addComma();
    json->addObjectEntry("product_list");
    json::toJSON(product_list, json);
    json->addComma();
    json->addObjectEntry("clicked_products");
    json::toJSON(clicked_products, json);
    json->endObject();
  }
}

void ECommerceSearchQueriesFeed::onFinish() {
  output_->json()->endArray();
}

String ECommerceSearchQueriesFeed::contentType() const {
  return "application/json; charset=utf-8";
}

} // namespace cm

