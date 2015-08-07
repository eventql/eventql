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
#include <cstable/BitPackedIntColumnWriter.h>

using namespace stx;

UNIT_TEST(CSTableTest);

TEST_CASE(CSTableTest, TestCSTableWriter, [] () {
  FileUtil::rm("/tmp/__fnord__cstabletest1.sstable");

  RefPtr<cstable::ColumnWriter> columns_;

  auto tbl = cstable::CSTableWriter(
    "/tmp/__fnord__cstabletest1.sstable",
    10);

  //tbl->addColumn("date",  new cstable::BitPackedIntColumnWriter(10, 10));
});
