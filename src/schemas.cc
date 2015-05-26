/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "schemas.h"

using namespace cm;
using namespace fnord;

namespace cm {

msg::MessageSchema joinedSessionsSchema() {
  Vector<msg::MessageSchemaField> fields;

  fields.emplace_back(
      60,
      "customer",
      msg::FieldType::STRING,
      250,
      false,
      true);

  fields.emplace_back(
      61,
      "customer_session_id",
      msg::FieldType::STRING,
      4096,
      false,
      true);

  fields.emplace_back(
      53,
      "first_seen_time",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      true);

  fields.emplace_back(
      54,
      "last_seen_time",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      true);

  fields.emplace_back(
      47,
      "num_cart_items",
      msg::FieldType::UINT32,
      250,
      false,
      true,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      48,
      "num_order_items",
      msg::FieldType::UINT32,
      250,
      false,
      true,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      49,
      "cart_value_eurcents",
      msg::FieldType::UINT32,
      0xffffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      50,
      "gmv_eurcents",
      msg::FieldType::UINT32,
      0xffffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      51,
      "ab_test_group",
      msg::FieldType::UINT32,
      100,
      false,
      true,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      56,
      "experiments",
      msg::FieldType::STRING,
      8192,
      false,
      true);

  fields.emplace_back(
      57,
      "referrer_url",
      msg::FieldType::STRING,
      1024,
      false,
      true);

  fields.emplace_back(
      58,
      "referrer_campaign",
      msg::FieldType::STRING,
      4096,
      false,
      true);

  fields.emplace_back(
      59,
      "referrer_name",
      msg::FieldType::STRING,
      255,
      false,
      true);

  msg::MessageSchemaField queries(
      16,
      "search_queries",
      msg::FieldType::OBJECT,
      0,
      true,
      false);

  queries.fields.emplace_back(
      18,
      "time",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      false);

  queries.fields.emplace_back(
      1,
      "page",
      msg::FieldType::UINT32,
      100,
      false,
      true,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      2,
      "language",
      msg::FieldType::UINT32,
      kMaxLanguage,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      3,
      "query_string",
      msg::FieldType::STRING,
      8192,
      false,
      true);

  queries.fields.emplace_back(
      4,
      "query_string_normalized",
      msg::FieldType::STRING,
      8192,
      false,
      true);

  queries.fields.emplace_back(
      5,
      "num_result_items",
      msg::FieldType::UINT32,
      250,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      6,
      "num_result_items_clicked",
      msg::FieldType::UINT32,
      250,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      7,
      "num_ad_impressions",
      msg::FieldType::UINT32,
      250,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      8,
      "num_ad_clicks",
      msg::FieldType::UINT32,
      250,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      43,
      "num_cart_items",
      msg::FieldType::UINT32,
      250,
      false,
      true,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      44,
      "num_order_items",
      msg::FieldType::UINT32,
      250,
      false,
      true,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      45,
      "cart_value_eurcents",
      msg::FieldType::UINT32,
      0xffffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  queries.fields.emplace_back(
      46,
      "gmv_eurcents",
      msg::FieldType::UINT32,
      0xffffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  queries.fields.emplace_back(
      9,
      "ab_test_group",
      msg::FieldType::UINT32,
      100,
      false,
      true,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      55,
      "experiments",
      msg::FieldType::STRING,
      8192,
      false,
      true);

  queries.fields.emplace_back(
      10,
      "device_type",
      msg::FieldType::UINT32,
      kMaxDeviceType,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      11,
      "page_type",
      msg::FieldType::UINT32,
      kMaxPageType,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      12,
      "category1",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  queries.fields.emplace_back(
      13,
      "category2",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  queries.fields.emplace_back(
      14,
      "category3",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  queries.fields.emplace_back(
      32,
      "shop_id",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  msg::MessageSchemaField query_items(
      17,
      "result_items",
      msg::FieldType::OBJECT,
      0,
      true,
      false);

  query_items.fields.emplace_back(
      19,
      "position",
      msg::FieldType::UINT32,
      64,
      false,
      false,
      msg::EncodingHint::BITPACK);

  query_items.fields.emplace_back(
      15,
      "clicked",
      msg::FieldType::BOOLEAN,
      0,
      false,
      false);

  query_items.fields.emplace_back(
      20,
      "item_id",
      msg::FieldType::STRING,
      1024,
      false,
      false);

  query_items.fields.emplace_back(
      21,
      "shop_id",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  query_items.fields.emplace_back(
      22,
      "category1",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  query_items.fields.emplace_back(
      23,
      "category2",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  query_items.fields.emplace_back(
      24,
      "category3",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  query_items.fields.emplace_back(
      62,
      "is_paid_result",
      msg::FieldType::BOOLEAN,
      0,
      false,
      true);

  query_items.fields.emplace_back(
      63,
      "is_recommendation",
      msg::FieldType::BOOLEAN,
      0,
      false,
      true);

  queries.fields.emplace_back(query_items);
  fields.emplace_back(queries);

  msg::MessageSchemaField item_visits(
      25,
      "item_visits",
      msg::FieldType::OBJECT,
      0,
      true,
      false);

  item_visits.fields.emplace_back(
      26,
      "time",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      false);

  item_visits.fields.emplace_back(
      27,
      "item_id",
      msg::FieldType::STRING,
      1024,
      false,
      false);

  item_visits.fields.emplace_back(
      28,
      "shop_id",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  item_visits.fields.emplace_back(
      29,
      "category1",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  item_visits.fields.emplace_back(
      30,
      "category2",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  item_visits.fields.emplace_back(
      31,
      "category3",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(item_visits);

  msg::MessageSchemaField cart_items(
      52,
      "cart_items",
      msg::FieldType::OBJECT,
      0,
      true,
      false);

  cart_items.fields.emplace_back(
      33,
      "time",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      false);

  cart_items.fields.emplace_back(
      34,
      "item_id",
      msg::FieldType::STRING,
      1024,
      false,
      false);

  cart_items.fields.emplace_back(
      35,
      "shop_id",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  cart_items.fields.emplace_back(
      36,
      "category1",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  cart_items.fields.emplace_back(
      37,
      "category2",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  cart_items.fields.emplace_back(
      38,
      "category3",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  cart_items.fields.emplace_back(
      39,
      "quantity",
      msg::FieldType::UINT32,
      0xffff,
      false,
      false,
      msg::EncodingHint::LEB128);

  cart_items.fields.emplace_back(
      40,
      "price_cents",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      false,
      msg::EncodingHint::LEB128);

  cart_items.fields.emplace_back(
      41,
      "currency",
      msg::FieldType::UINT32,
      kMaxCurrency,
      false,
      false,
      msg::EncodingHint::BITPACK);

  cart_items.fields.emplace_back(
      42,
      "checkout_step",
      msg::FieldType::UINT32,
      32,
      false,
      false,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(cart_items);

  return msg::MessageSchema("JoinedSession", fields);
}

msg::MessageSchema indexChangeRequestSchema() {
  Vector<msg::MessageSchemaField> fields;

  fields.emplace_back(
      1,
      "customer",
      msg::FieldType::STRING,
      250,
      false,
      false);

  fields.emplace_back(
      2,
      "docid",
      msg::FieldType::STRING,
      1024,
      false,
      false);

  msg::MessageSchemaField attributes(
      3,
      "attributes",
      msg::FieldType::OBJECT,
      0,
      true,
      false);

  attributes.fields.emplace_back(
      4,
      "key",
      msg::FieldType::STRING,
      1024,
      false,
      false);

  attributes.fields.emplace_back(
      5,
      "value",
      msg::FieldType::STRING,
      0xffffffff,
      false,
      false);

  fields.emplace_back(attributes);
  return msg::MessageSchema("IndexChangeRequest", fields);
}



}
