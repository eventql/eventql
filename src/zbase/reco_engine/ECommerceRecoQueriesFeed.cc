/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "ECommerceRecoQueriesFeed.h"

using namespace stx;

namespace cm {

ECommerceRecoQueriesFeed::ECommerceRecoQueriesFeed(
    RefPtr<TSDBTableScanSource<JoinedSession>> input,
    RefPtr<JSONSink> output) :
    ReportRDD(input.get(), output.get()),
    input_(input),
    output_(output),
    n_(0) {}

void ECommerceRecoQueriesFeed::onInit() {
  input_->forEach(
      std::bind(
          &ECommerceRecoQueriesFeed::onSession,
          this,
          std::placeholders::_1));

  output_->json()->beginArray();
}

void ECommerceRecoQueriesFeed::onSession(const JoinedSession& session) {
  auto json = output_->json();

  for (const auto& q : session.search_queries()) {
    switch (q.page_type()) {
      case PAGETYPE_SEARCH_PAGE:
      case PAGETYPE_CATALOG_PAGE:
        break;

      default:
        continue;
    }

    Set<String> product_list;
    Set<String> clicked_products;

    for (const auto& item : q.result_items()) {
      if (!item.is_recommendation()) {
        continue;
      }

      product_list.emplace(item.item_id());
      if (item.clicked()) {
        clicked_products.emplace(item.item_id());
      }
    }

    if (product_list.empty()){
      continue;
    }

    if (++n_ > 1) {
      json->addComma();
    }

    json->beginObject();
    json->addObjectEntry("time");
    json->addInteger(q.time());
    json->addComma();
    json->addObjectEntry("session_id");
    json->addString(session.customer_session_id());
    json->addComma();
    json->addObjectEntry("source");
    json->addString(q.query_type());
    json->addComma();
    json->addObjectEntry("ab_test_group");
    json->addString(StringUtil::toString(session.ab_test_group()));
    json->addComma();
    json->addObjectEntry("product_list");
    json::toJSON(product_list, json);
    json->addComma();
    json->addObjectEntry("clicked_products");
    json::toJSON(clicked_products, json);
    json->endObject();
  }
}

void ECommerceRecoQueriesFeed::onFinish() {
  output_->json()->endArray();
}

String ECommerceRecoQueriesFeed::contentType() const {
  return "application/json; charset=utf-8";
}

} // namespace cm

