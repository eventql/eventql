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
#include "eventql/util/test/unittest.h"
#include "eventql/util/util/PersistentHashSet.h"

#include "eventql/eventql.h"

UNIT_TEST(RecordIDSetTest);

TEST_CASE(RecordIDSetTest, TestInsertIntoEmptySet, [] () {
  FileUtil::rm("/tmp/_fnord_testrecidset.idx");
  PersistentHashSet recset("/tmp/_fnord_testrecidset.idx");

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
    PersistentHashSet recset("/tmp/_fnord_testrecidset.idx");
    EXPECT_FALSE(recset.hasRecordID(SHA1::compute("0x42424242")));
    EXPECT_FALSE(recset.hasRecordID(SHA1::compute("0x23232323")));
    EXPECT_FALSE(recset.hasRecordID(SHA1::compute("0x52525252")));

    EXPECT_EQ(recset.fetchRecordIDs().size(), 0);
    recset.addRecordID(SHA1::compute("0x42424242"));
    recset.addRecordID(SHA1::compute("0x23232323"));
  }

  {
    PersistentHashSet recset("/tmp/_fnord_testrecidset.idx");
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
    PersistentHashSet recset("/tmp/_fnord_testrecidset.idx");
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
  PersistentHashSet recset("/tmp/_fnord_testrecidset.idx");

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

TEST_CASE(RecordIDSetTest, TestInsert10kRows, [] () {
  FileUtil::rm("/tmp/_fnord_testrecidset.idx");
  Random rnd;

  {
    PersistentHashSet recset("/tmp/_fnord_testrecidset.idx");
    for (int i = 0; i < 10000; ++i) {
      recset.addRecordID(SHA1::compute(StringUtil::toString(i)));
    }
  }

  {
    Random rnd;
    PersistentHashSet recset("/tmp/_fnord_testrecidset.idx");

    auto ids = recset.fetchRecordIDs();
    EXPECT_EQ(ids.size(), 10000);
    for (int i = 0; i < 10000; ++i) {
      EXPECT_EQ(ids.count(SHA1::compute(StringUtil::toString(i))), 1);
    }
  }
});

