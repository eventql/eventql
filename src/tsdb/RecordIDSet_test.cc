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

/*

TEST_CASE(RecordSetTest, TestDuplicateRowsInCommitlog, [] () {
  auto schema = testSchema();
  RecordSet recset("/tmp", "_fnord_testrecset");

  recset.addRecord(SHA1::compute("0x42424242"), testObject(schema, "1a", "1b"));
  recset.addRecord(SHA1::compute("0x42424242"), testObject(schema, "2a", "2b"));
  EXPECT_EQ(recset.commitlogSize(), 1);

  recset.rollCommitlog();
  EXPECT_EQ(recset.commitlogSize(), 1);

  recset.addRecord(SHA1::compute("0x42424242"), testObject(schema, "3a", "3b"));
  recset.addRecord(SHA1::compute("0x32323232"), testObject(schema, "2a", "2b"));
  EXPECT_EQ(recset.commitlogSize(), 2);

  recset.rollCommitlog();
  recset.compact();
  EXPECT_EQ(recset.commitlogSize(), 0);

  auto res = recset.listRecords();
  EXPECT_EQ(res.size(), 2);
  EXPECT_EQ(res.count(SHA1::compute("x42424242")), 1);
  EXPECT_EQ(res.count(SHA1::compute("x32323232")), 1);
});

TEST_CASE(RecordSetTest, TestCompactionWithExistingTable, [] () {
  auto schema = testSchema();
  RecordSet recset("/tmp", "_fnord_testrecset");

  recset.addRecord(SHA1::compute("0x42424242"), testObject(schema, "1a", "1b"));
  recset.addRecord(SHA1::compute("0x23232323"), testObject(schema, "2a", "2b"));
  recset.rollCommitlog();
  recset.compact();

  recset.addRecord(SHA1::compute("0x52525252"), testObject(schema, "3a", "3b"));
  recset.addRecord(SHA1::compute("0x12121212"), testObject(schema, "4a", "4b"));
  recset.rollCommitlog();
  recset.compact();
  EXPECT_EQ(recset.commitlogSize(), 0);

  auto msgids = recset.listRecords();

  EXPECT_EQ(msgids.size(), 4);
  EXPECT_EQ(msgids.count(SHA1::compute("0x42424242")), 1);
  EXPECT_EQ(msgids.count(SHA1::compute("0x23232323")), 1);
  EXPECT_EQ(msgids.count(SHA1::compute("0x52525252")), 1);
  EXPECT_EQ(msgids.count(SHA1::compute("0x12121212")), 1);
});

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

TEST_CASE(RecordSetTest, TestSplitIntoMultipleDatafiles, [] () {
  Random rnd;
  auto schema = testSchema();
  RecordSet recset("/tmp", "_fnord_testrecset");
  recset.setMaxDatafileSize(1024 * 60);

  int n = 0;
  for (int j = 0; j < 10; ++j) {
    for (int i = 0; i < 1000; ++i) {
      recset.addRecord(
          SHA1::compute(StringUtil::toString(++n)),
          testObject(schema, "1a", "1b"));
    }

    recset.rollCommitlog();
    recset.compact();
    EXPECT_EQ(recset.commitlogSize(), 0);
  }

  EXPECT_EQ(recset.getState().datafiles.size(), 4);
  EXPECT_EQ(recset.getState().datafiles[0].num_records, 3000);
  EXPECT_EQ(recset.getState().datafiles[0].offset, 0);
  EXPECT_EQ(recset.getState().datafiles[1].num_records, 3000);
  EXPECT_EQ(recset.getState().datafiles[1].offset, 3000);
  EXPECT_EQ(recset.getState().datafiles[2].num_records, 3000);
  EXPECT_EQ(recset.getState().datafiles[2].offset, 6000);
  EXPECT_EQ(recset.getState().datafiles[3].num_records, 1000);
  EXPECT_EQ(recset.getState().datafiles[3].offset, 9000);

  auto msgids = recset.listRecords();
  EXPECT_EQ(msgids.size(), 10000);
  for (int i = 1; i <= 10000; ++i) {
    EXPECT_EQ(msgids.count(SHA1::compute(StringUtil::toString(i))), 1);
  }
});
*/

