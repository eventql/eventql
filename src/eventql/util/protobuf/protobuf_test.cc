/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include "eventql/util/HMAC.h"
#include "eventql/util/test/unittest.h"
#include "eventql/util/protobuf/DynamicMessage.h"

using namespace util;

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
            false),
        msg::MessageSchemaField(
            3,
            "three",
            msg::FieldType::STRING,
            0,
            true,
            false),
        msg::MessageSchemaField::mkObjectField(
            4,
            "four",
            true,
            false,
            new msg::MessageSchema(
                  "NestedTestSchema",
                  Vector<msg::MessageSchemaField> {
                      msg::MessageSchemaField(
                          1,
                          "alpha",
                          msg::FieldType::STRING,
                          250,
                          false,
                          false),
                      msg::MessageSchemaField(
                          2,
                          "beta",
                          msg::FieldType::DOUBLE,
                          0,
                          false,
                          false),
                  }))
      });
}

TEST_CASE(ProtobufTest, TestDynamicMessageToJSON, [] () {
  msg::DynamicMessage msg(testSchema());
  msg.addField("one", "fnord");
  msg.addField("two", "23.5");
  msg.addField("three", "blah");
  msg.addField("three", "fubar");

  Buffer json;
  json::JSONOutputStream jsons(BufferOutputStream::fromBuffer(&json));
  msg.toJSON(&jsons);

  EXPECT_EQ(
      json.toString(),
      R"({"one": "fnord","two": 23.5,"three": ["blah","fubar"]})");
});

TEST_CASE(ProtobufTest, TestNestedDynamicMessageToJSON, [] () {
  msg::DynamicMessage msg(testSchema());
  msg.addField("one", "fnord");
  msg.addObject("four", [] (msg::DynamicMessage* cld) {
    cld->addField("alpha", "a");
    cld->addField("beta", "123.5");
  });
  msg.addObject("four", [] (msg::DynamicMessage* cld) {
    cld->addField("alpha", "b");
    cld->addField("beta", "42");
  });

  Buffer json;
  json::JSONOutputStream jsons(BufferOutputStream::fromBuffer(&json));
  msg.toJSON(&jsons);

  EXPECT_EQ(
      json.toString(),
      R"({"one": "fnord","four": [{"alpha": "a","beta": 123.5},{"alpha": "b","beta": 42.0}]})");
});


TEST_CASE(ProtobufTest, TestDynamicMessageFromJSON, [] () {
  msg::DynamicMessage msg(testSchema());

  auto orig_json = R"({"one": "fnord","two": 23.5,"three": ["blah","fubar"]})";

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

TEST_CASE(ProtobufTest, TestNestedDynamicMessageFromJSON, [] () {
  msg::DynamicMessage msg(testSchema());

  auto orig_json = R"({"one": "fnord","four": [{"alpha": "a","beta": 123.5},{"alpha": "b","beta": 42.0}]})";

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
