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

TEST_CASE(CSTableTest, TestCSTableBitPacked, [] () {
  const String& filename = "/tmp/__fnord__cstabletest2.cstable";
  const uint64_t num_records = 1000;

  FileUtil::rm(filename);

  RefPtr<cstable::BitPackedIntColumnWriter> column_writer = mkRef(
    new cstable::BitPackedIntColumnWriter(10, 10));
  auto tbl_writer = cstable::CSTableWriter(
    filename,
    num_records);

  tbl_writer.addColumn("key1", column_writer.get());
  tbl_writer.commit();

  /*for (uint32_t i = 0; i < 10; i++) {
    column_writer->addDatum(1, 1, i);
  }*/

  column_writer->addDatum(1, 1, 10);

  cstable::CSTableReader tbl_reader(filename);
  RefPtr<cstable::ColumnReader> column_reader = tbl_reader.getColumnReader("key1");

  /*uint64_t rep_level;
  uint64_t def_level;
  void* data;
  size_t size;*/

  //column_reader->next(&rep_level, &def_level, &data, &size);
});

TEST_CASE(CSTableTest, TestBooleanColumnWriter, [] () {
  const String& filename = "/tmp/__fnord__cstabletest3.cstable";
  const uint64_t num_records = 1000;

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
  size_t size;

  for (uint32_t i = 0; i < 10; i++) {
    EXPECT_EQ(column_reader->next(&rep_level, &def_level, &data, &size), true);
  }
});




