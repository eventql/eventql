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
#include "eventql/util/protobuf/MessageDecoder.h"
#include "eventql/util/protobuf/MessageEncoder.h"
#include "eventql/core/RecordSet.h"

using namespace stx;
using namespace zbase;

UNIT_TEST(RecordSetTest);

RefPtr<msg::MessageSchema> testSchema() {
  Vector<msg::MessageSchemaField> fields;

  fields.emplace_back(
      1,
      "one",
      msg::FieldType::STRING,
      250,
      false,
      false);

  fields.emplace_back(
      2,
      "two",
      msg::FieldType::STRING,
      1024,
      false,
      false);

  return new msg::MessageSchema("TestSchema", fields);
}


Buffer testObject(RefPtr<msg::MessageSchema> schema, String one, String two) {
  msg::MessageObject obj;
  obj.addChild(1, one);
  obj.addChild(2, two);

  Buffer buf;
  msg::MessageEncoder::encode(obj, *schema, &buf);

  return buf;
}

TEST_CASE(RecordSetTest, TestAddRowToEmptySet, [] () {
  auto schema = testSchema();
  RecordSet recset("/tmp", "_fnord_testrecset");
  EXPECT_TRUE(recset.getState().commitlog.isEmpty());
  EXPECT_EQ(recset.getState().commitlog_size, 0);
  EXPECT_TRUE(recset.getState().datafiles.empty());
  EXPECT_EQ(recset.getState().old_commitlogs.size(), 0);

  recset.addRecord(SHA1::compute("0x42424242"), testObject(schema, "1a", "1b"));
  recset.addRecord(SHA1::compute("0x23232323"), testObject(schema, "2a", "2b"));

  EXPECT_FALSE(recset.getState().commitlog.isEmpty());
  EXPECT_TRUE(recset.getState().commitlog_size > 0);
  EXPECT_TRUE(recset.getState().datafiles.empty());
  EXPECT_EQ(recset.getState().old_commitlogs.size(), 0);
  EXPECT_EQ(recset.commitlogSize(), 2);

  recset.rollCommitlog();

  EXPECT_TRUE(recset.getState().commitlog.isEmpty());
  EXPECT_EQ(recset.getState().commitlog_size, 0);
  EXPECT_TRUE(recset.getState().datafiles.empty());
  EXPECT_EQ(recset.getState().old_commitlogs.size(), 1);
  EXPECT_EQ(recset.commitlogSize(), 2);

  recset.addRecord(SHA1::compute("0x1211111"), testObject(schema, "3a", "3b"));
  recset.addRecord(SHA1::compute("0x2344444"), testObject(schema, "4a", "4b"));

  EXPECT_FALSE(recset.getState().commitlog.isEmpty());
  EXPECT_TRUE(recset.getState().commitlog_size > 0);
  EXPECT_TRUE(recset.getState().datafiles.empty());
  EXPECT_EQ(recset.getState().old_commitlogs.size(), 1);
  EXPECT_EQ(recset.commitlogSize(), 4);

  recset.rollCommitlog();

  EXPECT_TRUE(recset.getState().commitlog.isEmpty());
  EXPECT_EQ(recset.getState().commitlog_size, 0);
  EXPECT_TRUE(recset.getState().datafiles.empty());
  EXPECT_EQ(recset.getState().old_commitlogs.size(), 2);
  EXPECT_EQ(recset.commitlogSize(), 4);

  recset.compact();
  EXPECT_TRUE(recset.getState().commitlog.isEmpty());
  EXPECT_EQ(recset.getState().commitlog_size, 0);
  EXPECT_EQ(recset.getState().datafiles.size(), 1);
  EXPECT_EQ(recset.getState().datafiles.back().num_records, 4);
  EXPECT_EQ(recset.getState().old_commitlogs.size(), 0);
  EXPECT_EQ(recset.commitlogSize(), 0);

  Set<String> res;
  recset.fetchRecords(0, 100, [&] (
      const SHA1Hash& id,
      const void* data,
      size_t size) {
    msg::MessageObject obj;
    msg::MessageDecoder::decode(data, size, *schema, &obj);
    res.emplace(obj.getString(1));
  });

  EXPECT_EQ(res.count("1a"), 1);
  EXPECT_EQ(res.count("2a"), 1);
  EXPECT_EQ(res.count("3a"), 1);
  EXPECT_EQ(res.count("4a"), 1);
});

TEST_CASE(RecordSetTest, TestCommitlogReopen, [] () {
  auto schema = testSchema();
  RecordSet::RecordSetState state;

  {
    RecordSet recset("/tmp", "_fnord_testrecset");
    recset.addRecord(SHA1::compute("0x42424242"), testObject(schema, "1a", "1b"));
    recset.addRecord(SHA1::compute("0x23232323"), testObject(schema, "2a", "2b"));
    state = recset.getState();
  }

  {
    RecordSet recset("/tmp", "_fnord_testrecset", state);
    EXPECT_EQ(recset.getState().commitlog_size, state.commitlog_size);
    EXPECT_EQ(recset.getState().old_commitlogs.size(), 0);
    EXPECT_EQ(recset.commitlogSize(), 2);
  }
});

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


