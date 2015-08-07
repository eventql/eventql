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


  RefPtr<cstable::ColumnReader> column_reader = tbl_reader.getColumnReader("key1");
});

TEST_CASE(CSTableTest, TestCSTable, [] () {
  const String& filename = "/tmp/__fnord__cstabletest2.cstable";
  const uint64_t num_records = 10;

  FileUtil::rm(filename);

  RefPtr<cstable::BitPackedIntColumnWriter> column_writer = mkRef(
    new cstable::BitPackedIntColumnWriter(10, 10));
  auto tbl_writer = cstable::CSTableWriter(
    filename,
    num_records);

  tbl_writer.addColumn("key1", column_writer.get());

  for (uint32_t i = 0; i < 1000; i++) {
    column_writer->addDatum(1, 1, i);
  }
});




