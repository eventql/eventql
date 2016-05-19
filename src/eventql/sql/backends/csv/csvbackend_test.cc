/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <eventql/util/exception.h>
#include <util/unittest.h>
#include <eventql/sql/backends/csv/csvinputstream.h>
#include <eventql/sql/backends/csv/csvtableref.h>
#include <eventql/sql/svalue.h>

using namespace csql::csv_backend;

UNIT_TEST(CSVInputStreamTest);

TEST_CASE(CSVInputStreamTest, TestOpenFile, [] () {
  auto csv_file = CSVInputStream::openFile(
      "test/fixtures/gbp_per_country.csv");

  EXPECT(csv_file.get() != nullptr);
});

TEST_CASE(CSVInputStreamTest, TestInvalidFileName, [] () {
  auto errmsg = "error opening file 'test/fixtures/invalid.csv': "
      "No such file or directory";

  EXPECT_EXCEPTION(errmsg, [] () {
    auto csv_file = CSVInputStream::openFile("test/fixtures/invalid.csv");
  });
});

TEST_CASE(CSVInputStreamTest, TestReadHeaders, [] () {
  auto csv_file = CSVInputStream::openFile(
      "test/fixtures/gbp_per_country_simple.csv");

  EXPECT(csv_file.get() != nullptr);
  std::vector<std::string> headers;
  csv_file->readNextRow(&headers);
  EXPECT_EQ(headers.size(), 2);
  EXPECT_EQ(headers[0], "country");
  EXPECT_EQ(headers[1], "gbp");
});

TEST_CASE(CSVInputStreamTest, TestReadSimpleRows, [] () {
  auto csv_file = CSVInputStream::openFile(
      "test/fixtures/gbp_per_country_simple.csv");
  EXPECT(csv_file.get() != nullptr);

  std::vector<std::string> headers;
  csv_file->readNextRow(&headers);
  EXPECT_EQ(headers.size(), 2);

  std::vector<std::string> row1;
  EXPECT(csv_file->readNextRow(&row1));
  EXPECT_EQ(row1.size(), 2);
  EXPECT_EQ(row1[0], "USA");
  EXPECT_EQ(row1[1], "16800000");

  std::vector<std::string> row2;
  EXPECT(csv_file->readNextRow(&row2));
  EXPECT_EQ(row2.size(), 2);
  EXPECT_EQ(row2[0], "CHN");
  EXPECT_EQ(row2[1], "9240270");
});

TEST_CASE(CSVInputStreamTest, TestReadSimpleRowsEOF, [] () {
  auto csv_file = CSVInputStream::openFile(
      "test/fixtures/gbp_per_country_simple.csv");
  EXPECT(csv_file.get() != nullptr);
  int num_rows = 0;

  for (;; ++num_rows) {
    std::vector<std::string> row;

    if (!csv_file->readNextRow(&row)) {
      break;
    }

    EXPECT_EQ(row.size(), 2);
  }

  EXPECT_EQ(num_rows, 192);
});

// CSVTableRefTest
TEST_CASE(CSVInputStreamTest, TestgetComputedColumnIndexWithHeaders, [] () {
  CSVTableRef table_ref(
      CSVInputStream::openFile("test/fixtures/gbp_per_country_simple.csv"),
      true);

  EXPECT_EQ(table_ref.getComputedColumnIndex("country"), 0);
});

// CSVTableRefTest
TEST_CASE(CSVInputStreamTest, TestReadRowsWithHeaders, [] () {
  CSVTableRef table_ref(
      CSVInputStream::openFile("test/fixtures/gbp_per_country_simple.csv"),
      true);

  for (int n = 0; n < 2; n++) {
    int num_rows = 0;
    for (;; ++num_rows) {
      std::vector<csql::SValue> target;
      if (!table_ref.readNextRow(&target)) {
        break;
      }
    }

    EXPECT_EQ(num_rows, 191);
    table_ref.rewind();
  }
});

// CSVTableRefTest
TEST_CASE(CSVInputStreamTest, TestgetComputedColumnIndexWithoutHeaders, [] () {
  CSVTableRef table_ref(
      CSVInputStream::openFile(
          "test/fixtures/gbp_per_country_simple_noheaders.csv"),
      false);

  EXPECT_EQ(table_ref.getComputedColumnIndex("col1"), 0);
  EXPECT_EQ(table_ref.getComputedColumnIndex("col2"), 1);
  EXPECT_EQ(table_ref.getComputedColumnIndex("col3"), 2);
  EXPECT_EQ(table_ref.getComputedColumnIndex("col99"), 98);
});

// CSVTableRefTest
TEST_CASE(CSVInputStreamTest, TestReadRowsWithoutHeaders, [] () {
  CSVTableRef table_ref(
      CSVInputStream::openFile(
          "test/fixtures/gbp_per_country_simple_noheaders.csv"),
      false);

  int num_rows = 0;
  for (;; ++num_rows) {
    std::vector<csql::SValue> target;
    if (!table_ref.readNextRow(&target)) {
      break;
    }
  }

  EXPECT_EQ(num_rows, 191);
});



