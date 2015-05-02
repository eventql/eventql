/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fnord-base/test/unittest.h"
#include "fnord-msg/MessageEncoder.h"
#include "fnord-tsdb/RecordSet.h"

using namespace fnord;
using namespace fnord::tsdb;

UNIT_TEST(RecordSetTest);

RefPtr<msg::MessageSchema> testSchema() {
  Vector<msg::MessageSchemaField> fields;

  fields.emplace_back(
      1,
      "one",
      msg::FieldType::STRING,
      250,
      false,
      false);

  fields.emplace_back(
      2,
      "two",
      msg::FieldType::STRING,
      1024,
      false,
      false);

  return new msg::MessageSchema("TestSchema", fields);
}


Buffer testObject(RefPtr<msg::MessageSchema> schema, String one, String two) {
  msg::MessageObject obj;
  obj.addChild(1, one);
  obj.addChild(2, two);

  Buffer buf;
  msg::MessageEncoder::encode(obj, *schema, &buf);

  return buf;
}

TEST_CASE(RecordSetTest, TestAddRowToEmptySet, [] () {
  auto schema = testSchema();
  RecordSet recset(schema, "/tmp/__fnord_testrecset");
  EXPECT_TRUE(recset.getState().commitlog.isEmpty());

  recset.addRecord(0x424242, testObject(schema, "1a", "1b"));
});
