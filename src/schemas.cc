/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "schemas.h"
#include "JoinedSession.pb.h"
#include "IndexChangeRequest.pb.h"

using namespace cm;
using namespace fnord;

namespace cm {

void loadDefaultSchemas(msg::MessageSchemaRepository* repo) {
  repo->registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedSession::descriptor()));

  repo->registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedSearchQuery::descriptor()));

  repo->registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedSearchQueryResultItem::descriptor()));

  repo->registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedCartItem::descriptor()));

  repo->registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedItemVisit::descriptor()));

  repo->registerSchema(
      msg::MessageSchema::fromProtobuf(IndexChangeRequest::descriptor()));
}

RefPtr<msg::MessageSchema> JoinedSessionSchema() {
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

  fields.emplace_back(
      msg::MessageSchemaField::mkObjectField(
        16,
        "search_queries",
        true,
        false,
        JoinedSearchQuerySchema()));

  fields.emplace_back(
      msg::MessageSchemaField::mkObjectField(
          52,
          "cart_items",
          true,
          false,
          JoinedCartItemSchema()));

  fields.emplace_back(
      msg::MessageSchemaField::mkObjectField(
          25,
          "item_visits",
          true,
          false,
          JoinedItemVisitSchema()));

  return new msg::MessageSchema("cm.JoinedSession", fields);
}

RefPtr<msg::MessageSchema> JoinedSearchQuerySchema() {
  Vector<msg::MessageSchemaField> fields;

  fields.emplace_back(
      18,
      "time",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      false);

  fields.emplace_back(
      1,
      "page",
      msg::FieldType::UINT32,
      100,
      false,
      true,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      2,
      "language",
      msg::FieldType::UINT32,
      kMaxLanguage,
      false,
      false,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      3,
      "query_string",
      msg::FieldType::STRING,
      8192,
      false,
      true);

  fields.emplace_back(
      4,
      "query_string_normalized",
      msg::FieldType::STRING,
      8192,
      false,
      true);

  fields.emplace_back(
      5,
      "num_result_items",
      msg::FieldType::UINT32,
      250,
      false,
      false,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      6,
      "num_result_items_clicked",
      msg::FieldType::UINT32,
      250,
      false,
      false,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      7,
      "num_ad_impressions",
      msg::FieldType::UINT32,
      250,
      false,
      false,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      8,
      "num_ad_clicks",
      msg::FieldType::UINT32,
      250,
      false,
      false,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      43,
      "num_cart_items",
      msg::FieldType::UINT32,
      250,
      false,
      true,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      44,
      "num_order_items",
      msg::FieldType::UINT32,
      250,
      false,
      true,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      45,
      "cart_value_eurcents",
      msg::FieldType::UINT32,
      0xffffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      46,
      "gmv_eurcents",
      msg::FieldType::UINT32,
      0xffffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      9,
      "ab_test_group",
      msg::FieldType::UINT32,
      100,
      false,
      true,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      55,
      "experiments",
      msg::FieldType::STRING,
      8192,
      false,
      true);

  fields.emplace_back(
      10,
      "device_type",
      msg::FieldType::UINT32,
      kMaxDeviceType,
      false,
      false,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      11,
      "page_type",
      msg::FieldType::UINT32,
      kMaxPageType,
      false,
      false,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      64,
      "query_type",
      msg::FieldType::STRING,
      1024,
      false,
      true);

  fields.emplace_back(
      12,
      "category1",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      13,
      "category2",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      14,
      "category3",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      32,
      "shop_id",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      msg::MessageSchemaField::mkObjectField(
          17,
          "result_items",
          true,
          false,
          JoinedSearchQueryResultItemSchema()));

  return new msg::MessageSchema("cm.JoinedSearchQuery", fields);
}

RefPtr<msg::MessageSchema> JoinedSearchQueryResultItemSchema() {
  Vector<msg::MessageSchemaField> fields;

  fields.emplace_back(
      19,
      "position",
      msg::FieldType::UINT32,
      64,
      false,
      false,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      15,
      "clicked",
      msg::FieldType::BOOLEAN,
      0,
      false,
      false);

  fields.emplace_back(
      65,
      "seen",
      msg::FieldType::BOOLEAN,
      0,
      false,
      true);

  fields.emplace_back(
      20,
      "item_id",
      msg::FieldType::STRING,
      1024,
      false,
      false);

  fields.emplace_back(
      21,
      "shop_id",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      22,
      "category1",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      23,
      "category2",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      24,
      "category3",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      62,
      "is_paid_result",
      msg::FieldType::BOOLEAN,
      0,
      false,
      true);

  fields.emplace_back(
      63,
      "is_recommendation",
      msg::FieldType::BOOLEAN,
      0,
      false,
      true);

  return new msg::MessageSchema("cm.JoinedSearchQueryResultItem", fields);
}

RefPtr<msg::MessageSchema> JoinedItemVisitSchema() {
  Vector<msg::MessageSchemaField> fields;

  fields.emplace_back(
      26,
      "time",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      false);

  fields.emplace_back(
      27,
      "item_id",
      msg::FieldType::STRING,
      1024,
      false,
      false);

  fields.emplace_back(
      28,
      "shop_id",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      29,
      "category1",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      30,
      "category2",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      31,
      "category3",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  return new msg::MessageSchema("cm.JoinedItemVisit", fields);
}

RefPtr<msg::MessageSchema> JoinedCartItemSchema() {
  Vector<msg::MessageSchemaField> fields;

  fields.emplace_back(
      33,
      "time",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      false);

  fields.emplace_back(
      34,
      "item_id",
      msg::FieldType::STRING,
      1024,
      false,
      false);

  fields.emplace_back(
      35,
      "shop_id",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      36,
      "category1",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      37,
      "category2",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      38,
      "category3",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      39,
      "quantity",
      msg::FieldType::UINT32,
      0xffff,
      false,
      false,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      40,
      "price_cents",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      false,
      msg::EncodingHint::LEB128);

  fields.emplace_back(
      41,
      "currency",
      msg::FieldType::UINT32,
      kMaxCurrency,
      false,
      false,
      msg::EncodingHint::BITPACK);

  fields.emplace_back(
      42,
      "checkout_step",
      msg::FieldType::UINT32,
      32,
      false,
      false,
      msg::EncodingHint::BITPACK);

  return new msg::MessageSchema("cm.JoinedCartItem", fields);
}

RefPtr<msg::MessageSchema> IndexChangeRequestSchema() {
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

  fields.emplace_back(
      msg::MessageSchemaField::mkObjectField(
          3,
          "attributes",
          true,
          false,
          IndexChangeRequestAttributeSchema()));

  return new msg::MessageSchema("cm.IndexChangeRequest", fields);
}

RefPtr<msg::MessageSchema> IndexChangeRequestAttributeSchema() {
  Vector<msg::MessageSchemaField> fields;

  fields.emplace_back(
      4,
      "key",
      msg::FieldType::STRING,
      1024,
      false,
      false);

  fields.emplace_back(
      5,
      "value",
      msg::FieldType::STRING,
      0xffffffff,
      false,
      false);

  return new msg::MessageSchema("cm.IndexChangeRequestAtttribute", fields);
}

}
