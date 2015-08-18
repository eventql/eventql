/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/json/json.h"
#include "zbase/logjoin/LogJoinExport.h"

using namespace stx;

namespace zbase {

LogJoinExport::LogJoinExport(
    http::HTTPConnectionPool* http) :
    broker_(http) {}

void LogJoinExport::exportSession(const JoinedSession& session) {
  exportPreferenceSetFeed(session);
}

void LogJoinExport::exportPreferenceSetFeed(const JoinedSession& session) {
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

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
  json.beginObject();
  json.addObjectEntry("visited_products");
  json::toJSON(visited_products, &json);
  json.addComma();
  json.addObjectEntry("bought_products");
  json::toJSON(bought_products, &json);
  json.endObject();

  auto topic = StringUtil::format(
      "logjoin.ecommerce_preference_sets.$0",
      session.customer());

  stx::iputs("export: $0 -> $1", topic, buf.toString());
}

void LogJoinExport::exportQueryFeed(const JoinedSession& session) {
}

} // namespace zbase
