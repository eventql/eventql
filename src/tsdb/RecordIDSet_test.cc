/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stx/test/unittest.h"
#include "tsdb/RecordIDSet.h"

using namespace stx;
using namespace tsdb;

UNIT_TEST(RecordIDSetTest);

TEST_CASE(RecordIDSetTest, TestInsertIntoEmptySet, [] () {
  FileUtil::rm("/tmp/_fnord_testrecidset.idx");
  RecordIDSet recset("/tmp/_fnord_testrecidset.idx");

  EXPECT_FALSE(recset.hasRecordID(SHA1::compute("0x42424242")));
  EXPECT_FALSE(recset.hasRecordID(SHA1::compute("0x23232323")));
  EXPECT_FALSE(recset.hasRecordID(SHA1::compute("0x52525252")));

  EXPECT_EQ(recset.fetchRecordIDs().size(), 0);

  recset.addRecordID(SHA1::compute("0x42424242"));
  recset.addRecordID(SHA1::compute("0x23232323"));

  EXPECT_TRUE(recset.hasRecordID(SHA1::compute("0x42424242")));
  EXPECT_TRUE(recset.hasRecordID(SHA1::compute("0x23232323")));
  EXPECT_FALSE(recset.hasRecordID(SHA1::compute("0x52525252")));

  auto ids = recset.fetchRecordIDs();
  EXPECT_EQ(ids.size(), 2);
  EXPECT_EQ(ids.count(SHA1::compute("0x42424242")), 1);
  EXPECT_EQ(ids.count(SHA1::compute("0x23232323")), 1);
});

TEST_CASE(RecordIDSetTest, TestReopen, [] () {
  FileUtil::rm("/tmp/_fnord_testrecidset.idx");

  {
    RecordIDSet recset("/tmp/_fnord_testrecidset.idx");
    EXPECT_FALSE(recset.hasRecordID(SHA1::compute("0x42424242")));
    EXPECT_FALSE(recset.hasRecordID(SHA1::compute("0x23232323")));
    EXPECT_FALSE(recset.hasRecordID(SHA1::compute("0x52525252")));

    EXPECT_EQ(recset.fetchRecordIDs().size(), 0);
    recset.addRecordID(SHA1::compute("0x42424242"));
    recset.addRecordID(SHA1::compute("0x23232323"));
  }

  {
    RecordIDSet recset("/tmp/_fnord_testrecidset.idx");
    EXPECT_TRUE(recset.hasRecordID(SHA1::compute("0x42424242")));
    EXPECT_TRUE(recset.hasRecordID(SHA1::compute("0x23232323")));
    EXPECT_FALSE(recset.hasRecordID(SHA1::compute("0x52525252")));

    auto ids = recset.fetchRecordIDs();
    EXPECT_EQ(ids.size(), 2);
    EXPECT_EQ(ids.count(SHA1::compute("0x42424242")), 1);
    EXPECT_EQ(ids.count(SHA1::compute("0x23232323")), 1);

    recset.addRecordID(SHA1::compute("0x52525252"));
  }

  {
    RecordIDSet recset("/tmp/_fnord_testrecidset.idx");
    EXPECT_TRUE(recset.hasRecordID(SHA1::compute("0x42424242")));
    EXPECT_TRUE(recset.hasRecordID(SHA1::compute("0x23232323")));
    EXPECT_TRUE(recset.hasRecordID(SHA1::compute("0x52525252")));

    auto ids = recset.fetchRecordIDs();
    EXPECT_EQ(ids.size(), 3);
    EXPECT_EQ(ids.count(SHA1::compute("0x42424242")), 1);
    EXPECT_EQ(ids.count(SHA1::compute("0x23232323")), 1);
    EXPECT_EQ(ids.count(SHA1::compute("0x52525252")), 1);
  }
});

TEST_CASE(RecordIDSetTest, TestDuplicateRecordIDs, [] () {
  FileUtil::rm("/tmp/_fnord_testrecidset.idx");
  RecordIDSet recset("/tmp/_fnord_testrecidset.idx");

  recset.addRecordID(SHA1::compute("0x42424242"));
  recset.addRecordID(SHA1::compute("0x42424242"));
  EXPECT_EQ(recset.fetchRecordIDs().size(), 1);

  recset.addRecordID(SHA1::compute("0x42424242"));
  recset.addRecordID(SHA1::compute("0x32323232"));
  EXPECT_EQ(recset.fetchRecordIDs().size(), 2);

  auto res = recset.fetchRecordIDs();
  EXPECT_EQ(res.size(), 2);
  EXPECT_EQ(res.count(SHA1::compute("0x42424242")), 1);
  EXPECT_EQ(res.count(SHA1::compute("0x32323232")), 1);
});

/*
TEST_CASE(RecordSetTest, TestInsert10kRows, [] () {
  Random rnd;
  auto schema = testSchema();
  RecordSet recset("/tmp", "_fnord_testrecset");

  for (int i = 0; i < 10; ++i) {
    for (int i = 0; i < 1000; ++i) {
      recset.addRecord(
          SHA1::compute(rnd.hex64()),
          testObject(schema, "1a", "1b"));
    }

    recset.rollCommitlog();
  }

  EXPECT_EQ(recset.getState().old_commitlogs.size(), 10);
  recset.compact();
  EXPECT_EQ(recset.commitlogSize(), 0);
  EXPECT_EQ(recset.getState().datafiles.size(), 1);
  EXPECT_EQ(recset.listRecords().size(), 10000);
});
*/

