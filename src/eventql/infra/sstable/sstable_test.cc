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
#include <eventql/util/stdtypes.h>
#include <eventql/util/io/file.h>
#include <eventql/util/test/unittest.h>
#include <eventql/infra/sstable/SSTableEditor.h>
#include <eventql/infra/sstable/SSTableWriter.h>
#include <eventql/infra/sstable/sstablereader.h>
#include <eventql/infra/sstable/rowoffsetindex.h>

using namespace sstable;
#include "eventql/eventql.h"
UNIT_TEST(SSTableTest);

TEST_CASE(SSTableTest, TestSSTableEditor, [] () {
  std::string header = "myfnordyheader!";
  IndexProvider indexes;

  FileUtil::rm("/tmp/__fnord__sstabletest1.sstable");
  auto tbl = SSTableEditor::create(
      "/tmp/__fnord__sstabletest1.sstable",
      std::move(indexes),
      header.data(),
      header.size());

  tbl->appendRow("key1", "value1");
  tbl->appendRow("key2", "value2");
  tbl->appendRow("key3", "value3");

  auto cursor = tbl->getCursor();
  EXPECT_EQ(cursor->getKeyString(), "key1");
  EXPECT_EQ(cursor->getDataString(), "value1");
  EXPECT_EQ(cursor->next(), true);

  EXPECT_EQ(cursor->getKeyString(), "key2");
  EXPECT_EQ(cursor->getDataString(), "value2");
  EXPECT_EQ(cursor->next(), true);

  EXPECT_EQ(cursor->getKeyString(), "key3");
  EXPECT_EQ(cursor->getDataString(), "value3");
  EXPECT_EQ(cursor->next(), false);

  tbl->appendRow("key4", "value4");

  EXPECT_EQ(cursor->next(), true);
  EXPECT_EQ(cursor->getKeyString(), "key4");
  EXPECT_EQ(cursor->getDataString(), "value4");
  EXPECT_EQ(cursor->next(), false);

  tbl->finalize();
});

TEST_CASE(SSTableTest, TestSSTableEditorWithIndexes, [] () {
  std::string header = "myfnordyheader!";

  IndexProvider indexes;
  indexes.addIndex<RowOffsetIndex>();

  FileUtil::rm("/tmp/__fnord__sstabletest2.sstable");
  auto tbl = SSTableEditor::create(
      "/tmp/__fnord__sstabletest2.sstable",
      std::move(indexes),
      header.data(),
      header.size());

  tbl->appendRow("key1", "value1");
  tbl->appendRow("key2", "value2");
  tbl->appendRow("key3", "value3");

  auto cursor = tbl->getCursor();
  EXPECT_EQ(cursor->getKeyString(), "key1");
  EXPECT_EQ(cursor->getDataString(), "value1");
  EXPECT_EQ(cursor->next(), true);

  EXPECT_EQ(cursor->getKeyString(), "key2");
  EXPECT_EQ(cursor->getDataString(), "value2");
  EXPECT_EQ(cursor->next(), true);

  EXPECT_EQ(cursor->getKeyString(), "key3");
  EXPECT_EQ(cursor->getDataString(), "value3");
  EXPECT_EQ(cursor->next(), false);

  tbl->appendRow("key4", "value4");

  EXPECT_EQ(cursor->next(), true);
  EXPECT_EQ(cursor->getKeyString(), "key4");
  EXPECT_EQ(cursor->getDataString(), "value4");
  EXPECT_EQ(cursor->next(), false);

  tbl->finalize();
});


TEST_CASE(SSTableTest, TestSSTableWriteThenRead, [] () {
  FileUtil::rm("/tmp/__fnord__sstabletest1.sstable");

  {
    std::string header = "myfnordyheader!";
    auto tbl = SSTableWriter::create(
        "/tmp/__fnord__sstabletest1.sstable",
        header.data(),
        header.size());

    tbl->appendRow("key1", "value1");
    tbl->appendRow("key2", "value2");
    tbl->appendRow("key3", "value3");
    tbl->commit();
  }

  {
    SSTableReader tbl(String("/tmp/__fnord__sstabletest1.sstable"));

    auto cursor = tbl.getCursor();
    EXPECT_EQ(cursor->getKeyString(), "key1");
    EXPECT_EQ(cursor->getDataString(), "value1");
    EXPECT_EQ(cursor->next(), true);

    EXPECT_EQ(cursor->getKeyString(), "key2");
    EXPECT_EQ(cursor->getDataString(), "value2");
    EXPECT_EQ(cursor->next(), true);

    EXPECT_EQ(cursor->getKeyString(), "key3");
    EXPECT_EQ(cursor->getDataString(), "value3");
    EXPECT_EQ(cursor->next(), false);
  });


TEST_CASE(SSTableTest, TestSSTableWriteReopenThenRead, [] () {
  FileUtil::rm("/tmp/__fnord__sstabletest1.sstable");

  {
    std::string header = "myfnordyheader!";
    auto tbl = SSTableWriter::create(
        "/tmp/__fnord__sstabletest1.sstable",
        header.data(),
        header.size());

    tbl->appendRow("key1", "value1");
    tbl->appendRow("key2", "value2");
    tbl->appendRow("key3", "value3");
    tbl->commit();
  }

  {
    auto tbl = SSTableWriter::reopen(
        "/tmp/__fnord__sstabletest1.sstable");

    tbl->appendRow("key5", "value5");
    tbl->appendRow("key6", "value6");
    tbl->commit();
  }

  {
    SSTableReader tbl(String("/tmp/__fnord__sstabletest1.sstable"));

    auto cursor = tbl.getCursor();
    EXPECT_EQ(cursor->getKeyString(), "key1");
    EXPECT_EQ(cursor->getDataString(), "value1");
    EXPECT_EQ(cursor->next(), true);

    EXPECT_EQ(cursor->getKeyString(), "key2");
    EXPECT_EQ(cursor->getDataString(), "value2");
    EXPECT_EQ(cursor->next(), true);

    EXPECT_EQ(cursor->getKeyString(), "key3");
    EXPECT_EQ(cursor->getDataString(), "value3");
    EXPECT_EQ(cursor->next(), true);

    EXPECT_EQ(cursor->getKeyString(), "key5");
    EXPECT_EQ(cursor->getDataString(), "value5");
    EXPECT_EQ(cursor->next(), true);

    EXPECT_EQ(cursor->getKeyString(), "key6");
    EXPECT_EQ(cursor->getDataString(), "value6");
    EXPECT_EQ(cursor->next(), false);
  });


