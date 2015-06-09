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
#include "fnord-tsdb/StreamChunk.h"

using namespace fnord;
using namespace tsdb;

UNIT_TEST(StreamChunkTest);

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

TEST_CASE(StreamChunkTest, TestStreamKeyGeneration, [] () {
  auto schema = testSchema();
  StreamConfig props(schema);

  auto key1 = StreamChunk::streamChunkKeyFor(
      "fnord-metric",
      DateTime(1430667887 * kMicrosPerSecond),
      props);

  auto key2 = StreamChunk::streamChunkKeyFor(
      "fnord-metric",
      DateTime(1430667880 * kMicrosPerSecond),
      props);

  EXPECT_EQ(key1, "fnord-metric\x1b\xf0\xef\x98\xaa\x05");
  EXPECT_EQ(key1, key2);
});

