/**
 * This file is part of the "FnordStream" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * Licensed under the MIT license (see LICENSE).
 */
#include <stdlib.h>
#include <stdio.h>
#include "stx/exception.h"
#include "stx/io/inputstream.h"
#include "stx/test/unittest.h"

using namespace stx;

UNIT_TEST(FileInputStreamTest);

TEST_CASE(FileInputStreamTest, TestOpenFile, [] () {
  auto file = FileInputStream::openFile(
      "test/fixtures/gbp_per_country.csv");

  EXPECT(file.get() != nullptr);
});

TEST_CASE(FileInputStreamTest, TestInvalidFileName, [] () {
  auto errmsg = "error opening file 'test/fixtures/invalid.csv': "
      "No such file or directory";

  EXPECT_EXCEPTION(errmsg, [] () {
    auto file = FileInputStream::openFile("test/fixtures/invalid.csv");
  });
});

TEST_CASE(FileInputStreamTest, TestReadUTF8ByteOrderMark, [] () {
  auto file = FileInputStream::openFile(
      "test/fixtures/gbp_per_country.csv");

  EXPECT(file.get() != nullptr);
  EXPECT(file->readByteOrderMark() == FileInputStream::BOM_UTF8);
});
