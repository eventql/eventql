/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/HMAC.h"
#include "stx/test/unittest.h"
#include "stx/protobuf/DynamicMessage.h"

using namespace stx;

UNIT_TEST(ProtobufTest);

RefPtr<msg::MessageSchema> testSchema() {
  return new msg::MessageSchema(
      "TestSchema",
      Vector<msg::MessageSchemaField> {
        msg::MessageSchemaField(
            1,
            "one",
            msg::FieldType::STRING,
            250,
            false,
            false),
        msg::MessageSchemaField(
            2,
            "two",
            msg::FieldType::DOUBLE,
            0,
            false,
            false)
      });
}

TEST_CASE(ProtobufTest, TestDynamicMessageToJSON, [] () {
  msg::DynamicMessage msg(testSchema());
  msg.addField("one", "fnord");
  msg.addField("two", "23.5");

  Buffer json;
  json::JSONOutputStream jsons(BufferOutputStream::fromBuffer(&json));
  msg.toJSON(&jsons);

  EXPECT_EQ(
      json.toString(),
      R"({"one": "fnord","two": 23.5})");
});

TEST_CASE(ProtobufTest, TestDynamicMessageFromJSON, [] () {
  msg::DynamicMessage msg(testSchema());

  auto orig_json = R"({"one": "fnord","two": 23.5})";

  {
    auto j = json::parseJSON(orig_json);
    msg.fromJSON(j.begin(), j.end());
  }

  {
    Buffer json;
    json::JSONOutputStream jsons(BufferOutputStream::fromBuffer(&json));
    msg.toJSON(&jsons);
    EXPECT_EQ(json.toString(), orig_json);
  }
});
