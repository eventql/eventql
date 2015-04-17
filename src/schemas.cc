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

  msg::MessageSchemaField queries(
      16,
      "queries",
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
      "num_items",
      msg::FieldType::UINT32,
      250,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      6,
      "num_items_clicked",
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
      9,
      "ab_test_group",
      msg::FieldType::UINT32,
      100,
      false,
      true,
      msg::EncodingHint::BITPACK);

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
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      13,
      "category2",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      14,
      "category3",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::BITPACK);

  msg::MessageSchemaField query_items(
      17,
      "items",
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

  queries.fields.emplace_back(query_items);
  fields.emplace_back(queries);

  return msg::MessageSchema("joined_session", fields);
}

}
