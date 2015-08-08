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
  const String& filename = "/tmp/__fnord__cstabletest1.cstable";
  const uint64_t num_records = 10;

  FileUtil::rm(filename);

  RefPtr<cstable::BitPackedIntColumnWriter> column_writer = mkRef(
    new cstable::BitPackedIntColumnWriter(10, 10));
  auto tbl_writer = cstable::CSTableWriter(
    filename,
    num_records);

  tbl_writer.addColumn("key1", column_writer.get());
  tbl_writer.addColumn("key2", column_writer.get());
  tbl_writer.commit();

  cstable::CSTableReader tbl_reader(filename);
  EXPECT_EQ(tbl_reader.numRecords(), num_records);
  EXPECT_EQ(tbl_reader.hasColumn("key1"), true);
  EXPECT_EQ(tbl_reader.hasColumn("key2"), true);
});

TEST_CASE(CSTableTest, TestBitPackedIntColumnWriterReader, [] () {
  const String& filename = "/tmp/__fnord__cstabletest2.cstable";
  const uint64_t num_records = 100;

  FileUtil::rm(filename);

  RefPtr<cstable::BitPackedIntColumnWriter> column_writer = mkRef(
    new cstable::BitPackedIntColumnWriter(1, 1));

  uint32_t v = 10;
  for (uint32_t i = 0; i < 10; i++) {
    column_writer->addDatum(1, 1, &v, sizeof(v));
  }


  auto tbl_writer = cstable::CSTableWriter(
    filename,
    num_records);
  tbl_writer.addColumn("key1", column_writer.get());
  tbl_writer.commit();


  cstable::CSTableReader tbl_reader(filename);
  RefPtr<cstable::ColumnReader> column_reader = tbl_reader.getColumnReader("key1");

  EXPECT_EQ(column_reader->type() == msg::FieldType::UINT32, true);

  uint64_t rep_level;
  uint64_t def_level;
  void* data;
  size_t size;
  uint32_t* val;

  for (uint32_t i = 0; i < 10; i++) {
    EXPECT_EQ(column_reader->next(&rep_level, &def_level, &data, &size), true);
    EXPECT_EQ(size, sizeof(v));
    val = static_cast<uint32_t*>(data);
    EXPECT_EQ(*val, v);
  }
});


TEST_CASE(CSTableTest, TestBooleanColumnWriterReader, [] () {
  const String& filename = "/tmp/__fnord__cstabletest3.cstable";
  const uint64_t num_records = 100;

  FileUtil::rm(filename);

  RefPtr<cstable::BooleanColumnWriter> column_writer = mkRef(
    new cstable::BooleanColumnWriter(1, 1));

  uint8_t v = 1;
  for (uint32_t i = 0; i < 10; i++) {
    column_writer->addDatum(1, 1, &v, sizeof(v));
  }


  auto tbl_writer = cstable::CSTableWriter(
    filename,
    num_records);
  tbl_writer.addColumn("key1", column_writer.get());
  tbl_writer.commit();


  cstable::CSTableReader tbl_reader(filename);
  RefPtr<cstable::ColumnReader> column_reader = tbl_reader.getColumnReader("key1");

  EXPECT_EQ(column_reader->type() == msg::FieldType::BOOLEAN, true);

  uint64_t rep_level;
  uint64_t def_level;
  void* data;
  uint8_t* val;
  size_t size;

  for (uint32_t i = 0; i < 10; i++) {
    EXPECT_EQ(column_reader->next(&rep_level, &def_level, &data, &size), true);
    val = static_cast<uint8_t*>(data);
    EXPECT_EQ(size, sizeof(v));
    EXPECT_EQ(*val, v);
  }
});

TEST_CASE(CSTableTest, TestDoubleColumnWriterReader, [] () {
  const String& filename = "/tmp/__fnord__cstabletest4.cstable";
  const uint64_t num_records = 100;

  FileUtil::rm(filename);

  RefPtr<cstable::DoubleColumnWriter> column_writer = mkRef(
    new cstable::DoubleColumnWriter(1, 1));

  double v = 1.1;
  for (uint32_t i = 0; i < 10; i++) {
    column_writer->addDatum(1, 1, &v, sizeof(v));
  }


  auto tbl_writer = cstable::CSTableWriter(
    filename,
    num_records);
  tbl_writer.addColumn("key1", column_writer.get());
  tbl_writer.commit();


  cstable::CSTableReader tbl_reader(filename);
  RefPtr<cstable::ColumnReader> column_reader = tbl_reader.getColumnReader("key1");

  EXPECT_EQ(column_reader->type() == msg::FieldType::DOUBLE, true);

  uint64_t rep_level;
  uint64_t def_level;
  void* data;
  double* val;
  size_t size;

  for (uint32_t i = 0; i < 10; i++) {
    EXPECT_EQ(column_reader->next(&rep_level, &def_level, &data, &size), true);
    val = static_cast<double*>(data);
    EXPECT_EQ(size, sizeof(v));
    EXPECT_EQ(*val, v);
  }
});

TEST_CASE(CSTableTest, TestLEB128ColumnWriterReader, [] () {
  const String& filename = "/tmp/__fnord__cstabletest5.cstable";
  const uint64_t num_records = 100;

  FileUtil::rm(filename);

  RefPtr<cstable::LEB128ColumnWriter> column_writer = mkRef(
    new cstable::LEB128ColumnWriter(1, 1));

  uint64_t v = 10;
  for (uint32_t i = 0; i < 10; i++) {
    column_writer->addDatum(1, 1, &v, sizeof(v));
  }


  auto tbl_writer = cstable::CSTableWriter(
    filename,
    num_records);
  tbl_writer.addColumn("key1", column_writer.get());
  tbl_writer.commit();


  cstable::CSTableReader tbl_reader(filename);
  RefPtr<cstable::ColumnReader> column_reader = tbl_reader.getColumnReader("key1");

  EXPECT_EQ(column_reader->type() == msg::FieldType::UINT64, true);

  uint64_t rep_level;
  uint64_t def_level;
  void* data;
  uint64_t* val;
  size_t size;

  for (uint32_t i = 0; i < 10; i++) {
    EXPECT_EQ(column_reader->next(&rep_level, &def_level, &data, &size), true);
    val = static_cast<uint64_t*>(data);
    EXPECT_EQ(size, sizeof(v));
    EXPECT_EQ(*val, v);
  }
});


TEST_CASE(CSTableTest, TestStringColumnWriterReader, [] () {
  const String& filename = "/tmp/__fnord__cstabletest6.cstable";
  const uint64_t num_records = 100;

  FileUtil::rm(filename);

  RefPtr<cstable::StringColumnWriter> column_writer = mkRef(
    new cstable::StringColumnWriter(1, 1));

  const String& v = "value";
  for (uint32_t i = 0; i < 10; i++) {
    column_writer->addDatum(1, 1, &v, sizeof(v));
  }


  auto tbl_writer = cstable::CSTableWriter(
    filename,
    num_records);
  tbl_writer.addColumn("key1", column_writer.get());
  tbl_writer.commit();


  cstable::CSTableReader tbl_reader(filename);
  RefPtr<cstable::ColumnReader> column_reader = tbl_reader.getColumnReader("key1");

  EXPECT_EQ(column_reader->type() == msg::FieldType::STRING, true);

  uint64_t rep_level;
  uint64_t def_level;
  void* data;
  String* val;
  size_t size;

  for (uint32_t i = 0; i < 10; i++) {
    EXPECT_EQ(column_reader->next(&rep_level, &def_level, &data, &size), true);
    val = static_cast<String*>(data);
    EXPECT_EQ(size, sizeof(v));
    EXPECT_EQ(*val, v);
  }
});

TEST_CASE(CSTableTest, TestUInt32ColumnWriterReader, [] () {
  const String& filename = "/tmp/__fnord__cstabletest7.cstable";
  const uint64_t num_records = 100;

  FileUtil::rm(filename);

  RefPtr<cstable::UInt32ColumnWriter> column_writer = mkRef(
    new cstable::UInt32ColumnWriter(1, 1));

  uint32_t v = 1;
  for (uint32_t i = 0; i < 10; i++) {
    column_writer->addDatum(1, 1, &v, sizeof(v));
  }


  auto tbl_writer = cstable::CSTableWriter(
    filename,
    num_records);
  tbl_writer.addColumn("key1", column_writer.get());
  tbl_writer.commit();


  cstable::CSTableReader tbl_reader(filename);
  RefPtr<cstable::ColumnReader> column_reader = tbl_reader.getColumnReader("key1");

  EXPECT_EQ(column_reader->type() == msg::FieldType::UINT32, true);

  uint64_t rep_level;
  uint64_t def_level;
  void* data;
  uint32_t* val;
  size_t size;

  for (uint32_t i = 0; i < 10; i++) {
    EXPECT_EQ(column_reader->next(&rep_level, &def_level, &data, &size), true);
    val = static_cast<uint32_t*>(data);
    EXPECT_EQ(size, sizeof(v));
    EXPECT_EQ(*val, v);
  }
});

TEST_CASE(CSTableTest, TestUInt64ColumnWriterReader, [] () {
  const String& filename = "/tmp/__fnord__cstabletest8.cstable";
  const uint64_t num_records = 100;

  FileUtil::rm(filename);

  RefPtr<cstable::UInt64ColumnWriter> column_writer = mkRef(
    new cstable::UInt64ColumnWriter(1, 1));

  uint64_t v = 1;
  for (uint32_t i = 0; i < 10; i++) {
    column_writer->addDatum(1, 1, &v, sizeof(v));
  }


  auto tbl_writer = cstable::CSTableWriter(
    filename,
    num_records);
  tbl_writer.addColumn("key1", column_writer.get());
  tbl_writer.commit();


  cstable::CSTableReader tbl_reader(filename);
  RefPtr<cstable::ColumnReader> column_reader = tbl_reader.getColumnReader("key1");

  EXPECT_EQ(column_reader->type() == msg::FieldType::UINT64, true);

  uint64_t rep_level;
  uint64_t def_level;
  void* data;
  uint64_t* val;
  size_t size;

  for (uint32_t i = 0; i < 10; i++) {
    EXPECT_EQ(column_reader->next(&rep_level, &def_level, &data, &size), true);
    val = static_cast<uint64_t*>(data);
    EXPECT_EQ(size, sizeof(v));
    EXPECT_EQ(*val, v);
  }
});





