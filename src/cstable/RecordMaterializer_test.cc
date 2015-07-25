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
#include "stx/test/unittest.h"
#include "stx/protobuf/MessageEncoder.h"
#include "stx/protobuf/MessagePrinter.h"
#include "cstable/CSTableReader.h"
#include "cstable/CSTableBuilder.h"
#include "cstable/RecordMaterializer.h"
#include "cstable/StringColumnReader.h"

using namespace stx;
using namespace stx::cstable;
using namespace stx::msg;

UNIT_TEST(RecordMaterializerTest);

TEST_CASE(RecordMaterializerTest, TestSimpleReMaterialization, [] () {
  String testfile = "/tmp/__fnord_testcstablematerialization.cst";

  msg::MessageSchemaField level1(
      1,
      "level1",
      msg::FieldType::OBJECT,
      0,
      true,
      false);

  msg::MessageSchemaField level1_str(
      2,
      "str",
      msg::FieldType::STRING,
      1024,
      true,
      false);

  level1.schema = new MessageSchema(
      "Level1",
      Vector<msg::MessageSchemaField> { level1_str });

  msg::MessageSchema schema(
      "TestSchema",
      Vector<msg::MessageSchemaField> { level1 });

  msg::MessageObject sobj;
  auto& l1_a = sobj.addChild(1);
  l1_a.addChild(2, "fnord1");
  l1_a.addChild(2, "fnord2");

  auto& l1_b = sobj.addChild(1);
  l1_b.addChild(2, "fnord3");
  l1_b.addChild(2, "fnord4");

  cstable::CSTableBuilder builder(&schema);
  builder.addRecord(sobj);
  builder.write(testfile);

  cstable::CSTableReader reader(testfile);
  cstable::RecordMaterializer materializer(&schema, &reader);

  msg::MessageObject robj;
  materializer.nextRecord(&robj);

  EXPECT_EQ(robj.asObject().size(), 2);
  EXPECT_EQ(robj.asObject()[0].asObject().size(), 2);
  EXPECT_EQ(robj.asObject()[0].asObject()[0].asString(), "fnord1");
  EXPECT_EQ(robj.asObject()[0].asObject()[1].asString(), "fnord2");
  EXPECT_EQ(robj.asObject()[1].asObject().size(), 2);
  EXPECT_EQ(robj.asObject()[1].asObject()[0].asString(), "fnord3");
  EXPECT_EQ(robj.asObject()[1].asObject()[1].asString(), "fnord4");
});


TEST_CASE(RecordMaterializerTest, TestSimpleReMaterializationWithNull, [] () {
  String testfile = "/tmp/__fnord_testcstablematerialization.cst";

  msg::MessageSchemaField level1(
      1,
      "level1",
      msg::FieldType::OBJECT,
      0,
      true,
      false);

  msg::MessageSchemaField level1_str(
      2,
      "str",
      msg::FieldType::STRING,
      1024,
      true,
      false);

  level1.schema = new MessageSchema(
      "Level1",
      Vector<msg::MessageSchemaField> { level1_str });

  msg::MessageSchema schema(
      "TestSchema",
      Vector<msg::MessageSchemaField> { level1 });

  msg::MessageObject sobj;
  auto& l1_a = sobj.addChild(1);
  l1_a.addChild(2, "fnord1");
  l1_a.addChild(2, "fnord2");

  auto& l1_b = sobj.addChild(1);

  auto& l1_c = sobj.addChild(1);
  l1_c.addChild(2, "fnord3");
  l1_c.addChild(2, "fnord4");

  cstable::CSTableBuilder builder(&schema);
  builder.addRecord(sobj);
  builder.write(testfile);

  cstable::CSTableReader reader(testfile);
  cstable::RecordMaterializer materializer(&schema, &reader);

  msg::MessageObject robj;
  materializer.nextRecord(&robj);

  EXPECT_EQ(robj.asObject().size(), 3);
  EXPECT_EQ(robj.asObject()[0].asObject().size(), 2);
  EXPECT_EQ(robj.asObject()[0].asObject()[0].asString(), "fnord1");
  EXPECT_EQ(robj.asObject()[0].asObject()[1].asString(), "fnord2");
  EXPECT_EQ(robj.asObject()[1].asObject().size(), 0);
  EXPECT_EQ(robj.asObject()[2].asObject().size(), 2);
  EXPECT_EQ(robj.asObject()[2].asObject()[0].asString(), "fnord3");
  EXPECT_EQ(robj.asObject()[2].asObject()[1].asString(), "fnord4");
});


TEST_CASE(RecordMaterializerTest, TestReMatWithNonRepeatedParent, [] () {
  String testfile = "/tmp/__fnord_testcstablematerialization.cst";

  msg::MessageSchemaField level1(
      1,
      "level1",
      msg::FieldType::OBJECT,
      0,
      true,
      false);

  msg::MessageSchemaField level2(
      2,
      "level2",
      msg::FieldType::OBJECT,
      0,
      true,
      false);

  msg::MessageSchemaField level2_str(
      3,
      "str",
      msg::FieldType::STRING,
      1024,
      true,
      false);

  level2.schema = new MessageSchema(
      "Level2",
      Vector<msg::MessageSchemaField> { level2_str });

  level1.schema = new MessageSchema(
      "Level1",
      Vector<msg::MessageSchemaField> { level2 });

  msg::MessageSchema schema(
      "TestSchema",
      Vector<msg::MessageSchemaField> { level1 });

  msg::MessageObject sobj;
  auto& l1_a = sobj.addChild(1);
  auto& l2_aa = l1_a.addChild(2);
  l2_aa.addChild(3, "fnord1");
  l2_aa.addChild(3, "fnord2");
  auto& l2_ab = l1_a.addChild(2);
  l2_ab.addChild(3, "fnord3");
  l2_ab.addChild(3, "fnord4");
  auto& l1_b = sobj.addChild(1);
  auto& l1_c = sobj.addChild(1);
  auto& l2_ca = l1_c.addChild(2);
  auto& l2_cb = l1_c.addChild(2);
  l2_cb.addChild(3, "fnord5");
  l2_cb.addChild(3, "fnord6");

  cstable::CSTableBuilder builder(&schema);
  builder.addRecord(sobj);
  builder.write(testfile);

  cstable::CSTableReader reader(testfile);
  cstable::RecordMaterializer materializer(&schema, &reader);

  msg::MessageObject robj;
  materializer.nextRecord(&robj);

  EXPECT_EQ(robj.asObject().size(), 3);
  EXPECT_EQ(robj.asObject()[0].asObject().size(), 2);
  EXPECT_EQ(robj.asObject()[0].asObject()[0].asObject().size(), 2);
  EXPECT_EQ(robj.asObject()[0].asObject()[0].asObject()[0].asString(), "fnord1");
  EXPECT_EQ(robj.asObject()[0].asObject()[0].asObject()[1].asString(), "fnord2");
  EXPECT_EQ(robj.asObject()[0].asObject()[1].asObject().size(), 2);
  EXPECT_EQ(robj.asObject()[0].asObject()[1].asObject()[0].asString(), "fnord3");
  EXPECT_EQ(robj.asObject()[0].asObject()[1].asObject()[1].asString(), "fnord4");
  EXPECT_EQ(robj.asObject()[1].asObject().size(), 0);
  EXPECT_EQ(robj.asObject()[2].asObject().size(), 2);
  EXPECT_EQ(robj.asObject()[2].asObject()[0].asObject().size(), 0);
  EXPECT_EQ(robj.asObject()[2].asObject()[1].asObject().size(), 2);
  EXPECT_EQ(robj.asObject()[2].asObject()[1].asObject()[0].asString(), "fnord5");
  EXPECT_EQ(robj.asObject()[2].asObject()[1].asObject()[1].asString(), "fnord6");
});
