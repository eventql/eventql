/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Laura Schlimmer, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/stdtypes.h>
#include <stx/io/file.h>
#include <stx/test/unittest.h>
#include <cstable/columns/v1/BitPackedIntColumnWriter.h>
#include <cstable/columns/v1/BooleanColumnWriter.h>
#include <cstable/columns/v1/DoubleColumnWriter.h>
#include <cstable/columns/v1/LEB128ColumnWriter.h>
#include <cstable/columns/v1/StringColumnWriter.h>
#include <cstable/columns/v1/UInt32ColumnWriter.h>
#include <cstable/columns/v1/UInt64ColumnWriter.h>
#include <cstable/CSTableWriter.h>
#include <cstable/CSTableReader.h>
#include <cstable/RecordShredder.h>
#include <cstable/RecordMaterializer.h>

using namespace cstable;

using FileUtil = stx::FileUtil;
using MessageSchema = stx::msg::MessageSchema;
using DynamicMessage = stx::msg::DynamicMessage;

UNIT_TEST(CSTableTest);

TEST_CASE(CSTableTest, TestV1CSTableContainer, [] () {
  String filename = "/tmp/__fnord__cstabletest1.cstable";
  auto num_records = 10;

  FileUtil::rm(filename);

  Vector<cstable::ColumnConfig> columns;
  cstable::TableSchema schema;

  schema.addUnsignedInteger("key1");
  schema.addUnsignedInteger("key2");

  auto tbl_writer = cstable::CSTableWriter::createFile(
      filename,
      cstable::BinaryFormatVersion::v0_1_0,
      schema);

  tbl_writer->addRows(num_records);
  tbl_writer->commit();

  auto tbl_reader = cstable::CSTableReader::openFile(filename);
  EXPECT_EQ(tbl_reader->numRecords(), num_records);
  EXPECT_EQ(tbl_reader->hasColumn("key1"), true);
  EXPECT_EQ(tbl_reader->hasColumn("key2"), true);
});

TEST_CASE(CSTableTest, TestV1CSTableColumnWriterReader, [] () {
  String filename = "/tmp/__fnord__cstabletest2.cstable";
  auto num_records = 4000;
  uint64_t rep_max = 1;
  uint64_t def_max = 1;

  FileUtil::rm(filename);

  cstable::TableSchema schema;

  schema.addUnsignedIntegerArray(
      "bitpacked",
      true,
      cstable::ColumnEncoding::UINT32_BITPACKED);

  schema.addUnsignedIntegerArray(
      "boolean",
      true,
      cstable::ColumnEncoding::BOOLEAN_BITPACKED);

  schema.addFloatArray(
      "double",
      true,
      cstable::ColumnEncoding::FLOAT_IEEE754);

  schema.addUnsignedIntegerArray(
      "leb128",
      true,
      cstable::ColumnEncoding::UINT64_LEB128);

  schema.addStringArray(
      "string",
      true,
      cstable::ColumnEncoding::STRING_PLAIN);

  schema.addUnsignedIntegerArray(
      "uint32",
      true,
      cstable::ColumnEncoding::UINT32_PLAIN);

  schema.addUnsignedIntegerArray(
      "uint64",
      true,
      cstable::ColumnEncoding::UINT64_PLAIN);

  auto tbl_writer = cstable::CSTableWriter::createFile(
      filename,
      cstable::BinaryFormatVersion::v0_1_0,
      schema);

  auto bitpacked_writer = tbl_writer->getColumnWriter("bitpacked");
  auto boolean_writer = tbl_writer->getColumnWriter("boolean");
  auto double_writer = tbl_writer->getColumnWriter("double");
  auto leb128_writer = tbl_writer->getColumnWriter("leb128");
  auto string_writer = tbl_writer->getColumnWriter("string");
  auto uint32_writer = tbl_writer->getColumnWriter("uint32");
  auto uint64_writer = tbl_writer->getColumnWriter("uint64");

  for (auto i = 0; i < num_records; i++) {
    tbl_writer->addRow();
    bitpacked_writer->writeUnsignedInt(rep_max, def_max, i);
    boolean_writer->writeBoolean(rep_max, def_max, i % 2 == 0);
    double_writer->writeFloat(rep_max, def_max, i * 1.1);
    leb128_writer->writeUnsignedInt(rep_max, def_max, i + 12);
    string_writer->writeString(rep_max, def_max, StringUtil::format("x$0x", i));
    uint32_writer->writeUnsignedInt(rep_max, def_max, i * 5);
    uint64_writer->writeUnsignedInt(rep_max, def_max, i * 8);
  }

  tbl_writer->commit();

  auto tbl_reader = cstable::CSTableReader::openFile(filename);
  EXPECT_EQ(tbl_reader->numRecords(), num_records);

  auto bitpacked_reader = tbl_reader->getColumnReader("bitpacked");
  auto boolean_reader = tbl_reader->getColumnReader("boolean");
  auto double_reader = tbl_reader->getColumnReader("double");
  auto leb128_reader = tbl_reader->getColumnReader("leb128");
  auto string_reader = tbl_reader->getColumnReader("string");
  auto uint32_reader = tbl_reader->getColumnReader("uint32");
  auto uint64_reader = tbl_reader->getColumnReader("uint64");

  EXPECT_EQ(bitpacked_reader->type() == ColumnType::UNSIGNED_INT, true);
  EXPECT_EQ(boolean_reader->type() == ColumnType::BOOLEAN, true);
  EXPECT_EQ(double_reader->type() == ColumnType::FLOAT, true);
  EXPECT_EQ(leb128_reader->type() == ColumnType::UNSIGNED_INT, true);
  EXPECT_EQ(string_reader->type() == ColumnType::STRING, true);
  EXPECT_EQ(uint32_reader->type() == ColumnType::UNSIGNED_INT, true);
  EXPECT_EQ(uint64_reader->type() == ColumnType::UNSIGNED_INT, true);

  for (auto i = 0; i < num_records; i++) {
    uint64_t rlvl;
    uint64_t dlvl;

    {
      uint64_t val_uint;
      EXPECT_TRUE(bitpacked_reader->readUnsignedInt(&rlvl, &dlvl, &val_uint));
      EXPECT_EQ(val_uint, i);
    }

    {
      bool val_bool;
      EXPECT_TRUE(boolean_reader->readBoolean(&rlvl, &dlvl, &val_bool));
      EXPECT_EQ(val_bool, i % 2 == 0);
    }

    {
      double val_float;
      EXPECT_TRUE(double_reader->readFloat(&rlvl, &dlvl, &val_float));
      EXPECT_EQ(val_float, i * 1.1);
    }

    {
      uint64_t val_uint;
      EXPECT_TRUE(leb128_reader->readUnsignedInt(&rlvl, &dlvl, &val_uint));
      EXPECT_EQ(val_uint, i + 12);
    }

    {
      String val_str;
      EXPECT_TRUE(string_reader->readString(&rlvl, &dlvl, &val_str));
      EXPECT_EQ(val_str, StringUtil::format("x$0x", i));
    }

    {
      uint64_t val_uint;
      EXPECT_TRUE(uint32_reader->readUnsignedInt(&rlvl, &dlvl, &val_uint));
      EXPECT_EQ(val_uint, i * 5);
    }

    {
      uint64_t val_uint;
      EXPECT_TRUE(uint64_reader->readUnsignedInt(&rlvl, &dlvl, &val_uint));
      EXPECT_EQ(val_uint, i * 8);
    }
  }
});

TEST_CASE(CSTableTest, TestV2CSTableContainer, [] () {
  String filename = "/tmp/__fnord__cstabletest2.cstable";
  FileUtil::rm(filename);

  auto num_records = 32;

  cstable::TableSchema schema;
  schema.addUnsignedInteger("key1", true, ColumnEncoding::UINT64_PLAIN);
  schema.addUnsignedInteger("key2", true, ColumnEncoding::UINT64_PLAIN);

  auto tbl_writer = cstable::CSTableWriter::createFile(
      filename,
      schema);

  tbl_writer->addRows(num_records);
  tbl_writer->commit();

  auto tbl_reader = cstable::CSTableReader::openFile(filename);
  EXPECT_EQ(tbl_reader->numRecords(), num_records);
  EXPECT_EQ(tbl_reader->hasColumn("key1"), true);
  EXPECT_EQ(tbl_reader->hasColumn("key2"), true);
});

//TEST_CASE(CSTableTest, TestV2UInt64Plain, [] () {
//  String filename = "/tmp/__fnord__cstabletest3.cstable";
//  FileUtil::rm(filename);
//
//  TableSchema schema;
//  schema.addUnsignedInteger(
//      "mycol",
//      false,
//      cstable::ColumnEncoding::UINT64_PLAIN);
//
//  {
//    auto tbl_writer = cstable::CSTableWriter::createFile(filename, schema);
//    auto mycol = tbl_writer->getColumnWriter("mycol");
//
//    for (size_t i = 1; i < 10000; ++i) {
//      mycol->writeUnsignedInt(0, 0, 23 * i);
//      mycol->writeUnsignedInt(0, 0, 42 * i);
//      mycol->writeUnsignedInt(0, 0, 17 * i);
//    }
//
//    tbl_writer->commit();
//  }
//
//  {
//    auto tbl_reader = cstable::CSTableReader::openFile(filename);
//    auto mycol = tbl_reader->getColumnReader("mycol");
//
//    for (size_t i = 1; i < 10000; ++i) {
//      uint64_t rlevel;
//      uint64_t dlevel;
//      uint64_t val;
//      mycol->readUnsignedInt(&rlevel, &dlevel, &val);
//      EXPECT_EQ(val, 23 * i);
//      mycol->readUnsignedInt(&rlevel, &dlevel, &val);
//      EXPECT_EQ(val, 42 * i);
//      mycol->readUnsignedInt(&rlevel, &dlevel, &val);
//      EXPECT_EQ(val, 17 * i);
//    }
//  }
//});

TEST_CASE(CSTableTest, TestSimpleReMaterialization, [] () {
  String testfile = "/tmp/__fnord_testcstablematerialization.cst";

  TableSchema rs_level1;
  rs_level1.addStringArray("str");

  TableSchema rs_schema;
  rs_schema.addSubrecordArray("level1", rs_level1);

  stx::msg::MessageSchemaField level1(
      4,
      "level1",
      stx::msg::FieldType::OBJECT,
      0,
      true,
      false);

  stx::msg::MessageSchemaField level1_str(
      8,
      "str",
      stx::msg::FieldType::STRING,
      1024,
      true,
      false);

  level1.schema = new MessageSchema(
      "Level1",
      Vector<stx::msg::MessageSchemaField> { level1_str });

  auto schema = mkRef(new stx::msg::MessageSchema(
      "TestSchema",
      Vector<stx::msg::MessageSchemaField> { level1 }));

  DynamicMessage sobj(schema);
  sobj.addObject("level1", [] (DynamicMessage* msg) {
    msg->addStringField("str", "fnord1");
    msg->addStringField("str", "fnord2");
  });
  sobj.addObject("level1", [] (DynamicMessage* msg) {
    msg->addStringField("str", "fnord3");
    msg->addStringField("str", "fnord4");
  });

  FileUtil::rm(testfile);
  auto writer = cstable::CSTableWriter::createFile(
      testfile,
      cstable::BinaryFormatVersion::v0_1_0,
      rs_schema);

  cstable::RecordShredder shredder(writer.get());
  shredder.addRecordFromProtobuf(sobj);
  writer->commit();

  auto reader = cstable::CSTableReader::openFile(testfile);
  cstable::RecordMaterializer materializer(schema.get(), reader.get());

  stx::msg::MessageObject robj;
  materializer.nextRecord(&robj);

  EXPECT_EQ(robj.asObject().size(), 2);
  EXPECT_EQ(robj.asObject()[0].asObject().size(), 2);
  EXPECT_EQ(robj.asObject()[0].asObject()[0].asString(), "fnord1");
  EXPECT_EQ(robj.asObject()[0].asObject()[1].asString(), "fnord2");
  EXPECT_EQ(robj.asObject()[1].asObject().size(), 2);
  EXPECT_EQ(robj.asObject()[1].asObject()[0].asString(), "fnord3");
  EXPECT_EQ(robj.asObject()[1].asObject()[1].asString(), "fnord4");
});

TEST_CASE(CSTableTest, TestSimpleReMaterializationWithNull, [] () {
  String testfile = "/tmp/__fnord_testcstablematerialization.cst";

  stx::msg::MessageSchemaField level1(
      1,
      "level1",
      stx::msg::FieldType::OBJECT,
      0,
      true,
      false);

  stx::msg::MessageSchemaField level1_str(
      2,
      "str",
      stx::msg::FieldType::STRING,
      1024,
      true,
      false);

  level1.schema = new MessageSchema(
      "Level1",
      Vector<stx::msg::MessageSchemaField> { level1_str });

  stx::msg::MessageSchema schema(
      "TestSchema",
      Vector<stx::msg::MessageSchemaField> { level1 });

  stx::msg::MessageObject sobj;
  auto& l1_a = sobj.addChild(1);
  l1_a.addChild(2, "fnord1");
  l1_a.addChild(2, "fnord2");

  sobj.addChild(1);

  auto& l1_c = sobj.addChild(1);
  l1_c.addChild(2, "fnord3");
  l1_c.addChild(2, "fnord4");

  FileUtil::rm(testfile);
  auto writer = cstable::CSTableWriter::createFile(
      testfile,
      cstable::BinaryFormatVersion::v0_1_0,
      TableSchema::fromProtobuf(schema));

  cstable::RecordShredder shredder(writer.get());
  shredder.addRecordFromProtobuf(sobj, schema);
  writer->commit();

  auto reader = cstable::CSTableReader::openFile(testfile);
  cstable::RecordMaterializer materializer(&schema, reader.get());

  stx::msg::MessageObject robj;
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

TEST_CASE(CSTableTest, TestReMatWithNonRepeatedParent, [] () {
  String testfile = "/tmp/__fnord_testcstablematerialization.cst";

  stx::msg::MessageSchemaField level1(
      1,
      "level1",
      stx::msg::FieldType::OBJECT,
      0,
      true,
      false);

  stx::msg::MessageSchemaField level2(
      2,
      "level2",
      stx::msg::FieldType::OBJECT,
      0,
      true,
      false);

  stx::msg::MessageSchemaField level2_str(
      3,
      "str",
      stx::msg::FieldType::STRING,
      1024,
      true,
      false);

  level2.schema = new MessageSchema(
      "Level2",
      Vector<stx::msg::MessageSchemaField> { level2_str });

  level1.schema = new MessageSchema(
      "Level1",
      Vector<stx::msg::MessageSchemaField> { level2 });

  stx::msg::MessageSchema schema(
      "TestSchema",
      Vector<stx::msg::MessageSchemaField> { level1 });

  stx::msg::MessageObject sobj;
  auto& l1_a = sobj.addChild(1);
  auto& l2_aa = l1_a.addChild(2);
  l2_aa.addChild(3, "fnord1");
  l2_aa.addChild(3, "fnord2");
  auto& l2_ab = l1_a.addChild(2);
  l2_ab.addChild(3, "fnord3");
  l2_ab.addChild(3, "fnord4");
   sobj.addChild(1);
  auto& l1_c = sobj.addChild(1);
  l1_c.addChild(2);
  auto& l2_cb = l1_c.addChild(2);
  l2_cb.addChild(3, "fnord5");
  l2_cb.addChild(3, "fnord6");

  FileUtil::rm(testfile);
  auto writer = cstable::CSTableWriter::createFile(
      testfile,
      cstable::BinaryFormatVersion::v0_1_0,
      TableSchema::fromProtobuf(schema));

  cstable::RecordShredder shredder(writer.get());
  shredder.addRecordFromProtobuf(sobj, schema);
  writer->commit();

  auto reader = cstable::CSTableReader::openFile(testfile);
  cstable::RecordMaterializer materializer(&schema, reader.get());

  stx::msg::MessageObject robj;
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
