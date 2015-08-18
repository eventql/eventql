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
#include <cstable/CSTableWriter.h>
#include <cstable/CSTableReader.h>
#include <cstable/BitPackedIntColumnWriter.h>
#include <cstable/BooleanColumnWriter.h>
#include <cstable/DoubleColumnWriter.h>
#include <cstable/LEB128ColumnWriter.h>
#include <cstable/StringColumnWriter.h>
#include <cstable/UInt32ColumnWriter.h>
#include <cstable/UInt64ColumnWriter.h>


using namespace stx;

UNIT_TEST(CSTableTest);

TEST_CASE(CSTableTest, TestCSTableContainer, [] () {
  auto filename = "/tmp/__fnord__cstabletest1.cstable";
  auto num_records = 10;

  FileUtil::rm(filename);

  auto column1_writer = mkRef(new cstable::BitPackedIntColumnWriter(10, 10));
  auto column2_writer = mkRef(new cstable::BitPackedIntColumnWriter(10, 10));
  cstable::CSTableWriter tbl_writer(filename, num_records);

  tbl_writer.addColumn("key1", column1_writer.get());
  tbl_writer.addColumn("key2", column2_writer.get());
  tbl_writer.commit();

  cstable::CSTableReader tbl_reader(filename);
  EXPECT_EQ(tbl_reader.numRecords(), num_records);
  EXPECT_EQ(tbl_reader.hasColumn("key1"), true);
  EXPECT_EQ(tbl_reader.hasColumn("key2"), true);
});

TEST_CASE(CSTableTest, TestCSTableColumnWriterReader, [] () {
  auto filename = "/tmp/__fnord__cstabletest2.cstable";
  auto num_records = 4000;
  auto rep_max = 1;
  auto def_max = 1;

  FileUtil::rm(filename);

  auto bitpacked_writer = mkRef(
    new cstable::BitPackedIntColumnWriter(rep_max, def_max));
  auto boolean_writer = mkRef(
    new cstable::BooleanColumnWriter(rep_max, def_max));
  auto double_writer = mkRef(
    new cstable::DoubleColumnWriter(rep_max, def_max));
  auto leb128_writer = mkRef(
    new cstable::LEB128ColumnWriter(rep_max, def_max));
  auto string_writer = mkRef(
    new cstable::StringColumnWriter(rep_max, def_max));
  auto uint32_writer = mkRef(
    new cstable::UInt32ColumnWriter(rep_max, def_max));
  auto uint64_writer = mkRef(
    new cstable::UInt64ColumnWriter(rep_max, def_max));


  for (auto i = 0; i < num_records; i++) {
    uint8_t boolean_v = i % 2;
    double double_v = i * 1.1;
    uint64_t uint64_v = static_cast<uint64_t>(i);
    const String string_v = "value";

    bitpacked_writer->addDatum(rep_max, def_max, &i, sizeof(i));
    boolean_writer->addDatum(rep_max, def_max, &boolean_v, sizeof(boolean_v));
    double_writer->addDatum(rep_max, def_max, &double_v, sizeof(double_v));
    leb128_writer->addDatum(rep_max, def_max, &i, sizeof(i));
    string_writer->addDatum(rep_max, def_max, &string_v, sizeof(string_v));
    uint32_writer->addDatum(rep_max, def_max, &i, sizeof(i));
    uint64_writer->addDatum(rep_max, def_max, &uint64_v, sizeof(uint64_v));
  }

  cstable::CSTableWriter tbl_writer(filename, num_records);
  tbl_writer.addColumn("bitpacked", bitpacked_writer.get());
  tbl_writer.addColumn("boolean", boolean_writer.get());
  tbl_writer.addColumn("double", double_writer.get());
  tbl_writer.addColumn("leb128", leb128_writer.get());
  tbl_writer.addColumn("string", string_writer.get());
  tbl_writer.addColumn("uint32", uint32_writer.get());
  tbl_writer.addColumn("uint64", uint64_writer.get());
  tbl_writer.commit();

  cstable::CSTableReader tbl_reader(filename);
  auto bitpacked_reader = tbl_reader.getColumnReader("bitpacked");
  auto boolean_reader = tbl_reader.getColumnReader("boolean");
  auto double_reader = tbl_reader.getColumnReader("double");
  auto leb128_reader = tbl_reader.getColumnReader("leb128");
  auto string_reader = tbl_reader.getColumnReader("string");
  auto uint32_reader = tbl_reader.getColumnReader("uint32");
  auto uint64_reader = tbl_reader.getColumnReader("uint64");


  EXPECT_EQ(bitpacked_reader->type() == msg::FieldType::UINT32, true);
  EXPECT_EQ(boolean_reader->type() == msg::FieldType::BOOLEAN, true);
  EXPECT_EQ(double_reader->type() == msg::FieldType::DOUBLE, true);
  EXPECT_EQ(leb128_reader->type() == msg::FieldType::UINT64, true);
  EXPECT_EQ(string_reader->type() == msg::FieldType::STRING, true);
  EXPECT_EQ(uint32_reader->type() == msg::FieldType::UINT32, true);
  EXPECT_EQ(uint64_reader->type() == msg::FieldType::UINT64, true);


  for (auto i = 0; i < num_records; i++) {
    uint64_t rep_level;
    uint64_t def_level;
    size_t size;
    void* data;

    EXPECT_EQ(
      bitpacked_reader->next(&rep_level, &def_level, &data, &size),
      true);
    EXPECT_EQ(*static_cast<uint32_t*>(data), i);

    EXPECT_EQ(
      boolean_reader->next(&rep_level, &def_level, &data, &size),
      true);
    EXPECT_EQ(*static_cast<uint8_t*>(data), i % 2);

    EXPECT_EQ(
      double_reader->next(&rep_level, &def_level, &data, &size),
      true);
    EXPECT_EQ(*static_cast<double*>(data), i * 1.1);

    EXPECT_EQ(
      leb128_reader->next(&rep_level, &def_level, &data, &size),
      true);
    EXPECT_EQ(*static_cast<uint64_t*>(data), i);

    EXPECT_EQ(
      string_reader->next(&rep_level, &def_level, &data, &size),
      true);
    EXPECT_EQ(*static_cast<String*>(data), "value");

    EXPECT_EQ(
      uint32_reader->next(&rep_level, &def_level, &data, &size),
      true);
    EXPECT_EQ(*static_cast<uint32_t*>(data), i);

    EXPECT_EQ(
      uint64_reader->next(&rep_level, &def_level, &data, &size),
      true);
    EXPECT_EQ(*static_cast<uint64_t*>(data), static_cast<uint64_t>(i));
  }
});

