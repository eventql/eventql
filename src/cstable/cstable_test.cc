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

TEST_CASE(CSTableTest, TestCSTableWriter, [] () {
  const String& filename = "/tmp/__fnord__cstabletest1.cstable";
  const uint64_t num_records = 10;

  FileUtil::rm(filename);

  RefPtr<cstable::BitPackedIntColumnWriter> column_writer = mkRef(new cstable::BitPackedIntColumnWriter(10, 10));
  auto tbl_writer = cstable::CSTableWriter(
    filename,
    num_records);

  tbl_writer.addColumn("date", column_writer.get());
  tbl_writer.commit();

  cstable::CSTableReader cstable(filename);

  EXPECT_EQ(cstable.numRecords(), num_records);
});


