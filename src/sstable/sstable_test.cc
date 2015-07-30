/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/stdtypes.h>
#include <stx/io/file.h>
#include <stx/test/unittest.h>
#include <sstable/SSTableEditor.h>
#include <sstable/SSTableWriter.h>
#include <sstable/sstablereader.h>
#include <sstable/rowoffsetindex.h>

using namespace stx::sstable;
using namespace stx;
UNIT_TEST(SSTableTest);

TEST_CASE(SSTableTest, TestSSTableEditor, [] () {
  auto file = File::openFile(
      "/tmp/__fnord__sstabletest1.sstable",
      File::O_READ | File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

  std::string header = "myfnordyheader!";

  IndexProvider indexes;

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
  auto file = File::openFile(
      "/tmp/__fnord__sstabletest2.sstable",
      File::O_READ | File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

  std::string header = "myfnordyheader!";

  IndexProvider indexes;
  indexes.addIndex<RowOffsetIndex>();

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
    SSTableReader tbl("/tmp/__fnord__sstabletest1.sstable");

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
  }
});



