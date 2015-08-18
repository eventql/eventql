/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "SearchCTRStats.h"

#include "cstable/BitPackedIntColumnReader.h"

using namespace stx;

namespace zbase {

SearchCTRStats::SearchCTRStats() :
    num_sessions(0),
    num_queries(0),
    num_queries_clicked(0),
    num_query_items_clicked(0),
    num_query_item_impressions(0),
    num_ad_impressions(0),
    num_ads_clicked(0),
    query_num_cart_items(0),
    query_num_order_items(0),
    query_cart_value_eurcent(0),
    query_gmv_eurcent(0),
    query_ecommerce_conversions(0),
    query_abandoned_carts(0) {}

void SearchCTRStats::merge(const SearchCTRStats& other) {
  num_sessions += other.num_sessions;
  num_queries += other.num_queries;
  num_queries_clicked += other.num_queries_clicked;
  num_query_items_clicked += other.num_query_items_clicked;
  num_query_item_impressions += other.num_query_item_impressions;
  num_ad_impressions += other.num_ad_impressions;
  num_ads_clicked += other.num_ads_clicked;
  query_num_cart_items += other.query_num_cart_items;
  query_num_order_items += other.query_num_order_items;
  query_cart_value_eurcent += other.query_cart_value_eurcent;
  query_gmv_eurcent += other.query_gmv_eurcent;
  query_ecommerce_conversions += other.query_ecommerce_conversions;
  query_abandoned_carts += other.query_abandoned_carts;
}

void SearchCTRStats::encode(util::BinaryMessageWriter* writer) const {
  writer->appendUInt8(0x02);
  writer->appendVarUInt(num_sessions);
  writer->appendVarUInt(num_queries);
  writer->appendVarUInt(num_queries_clicked);
  writer->appendVarUInt(num_query_items_clicked);
  writer->appendVarUInt(num_query_item_impressions);
  writer->appendVarUInt(num_ad_impressions);
  writer->appendVarUInt(num_ads_clicked);
  writer->appendVarUInt(query_num_cart_items);
  writer->appendVarUInt(query_num_order_items);
  writer->appendVarUInt(query_cart_value_eurcent);
  writer->appendVarUInt(query_gmv_eurcent);
  writer->appendVarUInt(query_ecommerce_conversions);
  writer->appendVarUInt(query_abandoned_carts);
}

void SearchCTRStats::decode(util::BinaryMessageReader* reader) {
  if (*reader->readUInt8() != 0x02) {
    RAISE(kRuntimeError, "unsupported version");
  }

  num_sessions = reader->readVarUInt();
  num_queries = reader->readVarUInt();
  num_queries_clicked = reader->readVarUInt();
  num_query_items_clicked = reader->readVarUInt();
  num_query_item_impressions = reader->readVarUInt();
  num_ad_impressions = reader->readVarUInt();
  num_ads_clicked = reader->readVarUInt();
  query_num_cart_items = reader->readVarUInt();
  query_num_order_items = reader->readVarUInt();
  query_cart_value_eurcent = reader->readVarUInt();
  query_gmv_eurcent = reader->readVarUInt();
  query_ecommerce_conversions = reader->readVarUInt();
  query_abandoned_carts = reader->readVarUInt();
}

static double mkRate(double num, double div) {
  if (div == 0 || num == 0) {
    return 0;
  }

  return num / div;
}

void SearchCTRStats::toJSON(json::JSONOutputStream* json) const {
  json->beginObject();

  json->addObjectEntry("num_sessions");
  json->addInteger(num_sessions);
  json->addComma();

  json->addObjectEntry("num_queries");
  json->addInteger(num_queries);
  json->addComma();

  json->addObjectEntry("queries_per_session");
  json->addFloat(mkRate(num_queries, num_sessions));
  json->addComma();

  json->addObjectEntry("queries_clicked_per_session");
  json->addFloat(mkRate(num_queries_clicked, num_sessions));
  json->addComma();

  json->addObjectEntry("query_items_clicked_per_session");
  json->addFloat(mkRate(num_query_items_clicked, num_sessions));
  json->addComma();

  json->addObjectEntry("ad_impressions_per_session");
  json->addFloat(mkRate(num_ad_impressions, num_sessions));
  json->addComma();

  json->addObjectEntry("ad_clicks_per_session");
  json->addFloat(mkRate(num_ads_clicked, num_sessions));
  json->addComma();

  json->addObjectEntry("num_queries_clicked");
  json->addInteger(num_queries_clicked);
  json->addComma();

  json->addObjectEntry("num_query_items_clicked");
  json->addInteger(num_query_items_clicked);
  json->addComma();

  json->addObjectEntry("num_query_item_impressions");
  json->addInteger(num_query_item_impressions);
  json->addComma();

  json->addObjectEntry("num_ad_impressions");
  json->addInteger(num_ad_impressions);
  json->addComma();

  json->addObjectEntry("num_ads_clicked");
  json->addInteger(num_ads_clicked);
  json->addComma();

  json->addObjectEntry("query_ctr");
  json->addFloat(mkRate(num_queries_clicked, num_queries));
  json->addComma();

  json->addObjectEntry("query_cpq");
  json->addFloat(mkRate(num_query_items_clicked, num_queries));
  json->addComma();

  json->addObjectEntry("item_ctr");
  json->addFloat(mkRate(num_query_items_clicked, num_query_item_impressions));
  json->addComma();

  json->addObjectEntry("ad_ctr");
  json->addFloat(mkRate(num_ads_clicked, num_ad_impressions));
  json->addComma();

  json->addObjectEntry("ad_vs_organic_rate");
  json->addFloat(mkRate(num_ads_clicked, num_query_items_clicked));
  json->addComma();

  json->addObjectEntry("query_total_num_cart_items");
  json->addInteger(query_num_cart_items);
  json->addComma();

  json->addObjectEntry("query_num_cart_items");
  json->addInteger(mkRate(query_num_cart_items, num_queries));
  json->addComma();

  json->addObjectEntry("query_total_num_order_items");
  json->addInteger(query_num_order_items);
  json->addComma();

  json->addObjectEntry("query_num_order_items");
  json->addInteger(mkRate(query_num_order_items, num_queries));
  json->addComma();


  json->addObjectEntry("query_total_cart_value_eurcent");
  json->addInteger(query_cart_value_eurcent);
  json->addComma();

  json->addObjectEntry("query_cart_value_eurcent");
  json->addInteger(mkRate(query_cart_value_eurcent, num_queries));
  json->addComma();

  json->addObjectEntry("query_total_gmv_eurcent");
  json->addInteger(query_gmv_eurcent);
  json->addComma();

  json->addObjectEntry("query_gmv_eurcent");
  json->addInteger(mkRate(query_gmv_eurcent, num_queries));
  json->addComma();

  json->addObjectEntry("query_num_ecommerce_conversions");
  json->addInteger(query_ecommerce_conversions);
  json->addComma();

  json->addObjectEntry("query_ecommerce_conversion_rate");
  json->addFloat(mkRate(query_ecommerce_conversions, num_queries));
  json->addComma();

  json->addObjectEntry("query_num_abandoned_carts");
  json->addInteger(query_abandoned_carts);

  json->endObject();
}


} // namespace zbase

