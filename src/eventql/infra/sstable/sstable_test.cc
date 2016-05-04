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
#include <eventql/infra/sstable/SSTableEditor.h>
#include <eventql/infra/sstable/SSTableWriter.h>
#include <eventql/infra/sstable/sstablereader.h>
#include <eventql/infra/sstable/rowoffsetindex.h>

using namespace stx::sstable;
using namespace stx;
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
  }
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
  }
});


