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
#include "fnord-base/test/unittest.h"
#include "fnord-msg/MessageEncoder.h"
#include "fnord-tsdb/RecordSet.h"
#include "fnord-cstable/CSTableReader.h"
#include "fnord-cstable/StringColumnReader.h"

using namespace fnord;
using namespace fnord::tsdb;

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
  RecordSet recset(schema, "/tmp/__fnord_testrecset");
  EXPECT_TRUE(recset.getState().commitlog.isEmpty());
  EXPECT_EQ(recset.getState().commitlog_size, 0);
  EXPECT_TRUE(recset.getState().datafiles.empty());
  EXPECT_EQ(recset.getState().old_commitlogs.size(), 0);

  recset.addRecord(0x42424242, testObject(schema, "1a", "1b"));
  recset.addRecord(0x23232323, testObject(schema, "2a", "2b"));

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

  recset.addRecord(0x1211111, testObject(schema, "3a", "3b"));
  recset.addRecord(0x2344444, testObject(schema, "4a", "4b"));

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
  EXPECT_EQ(recset.getState().old_commitlogs.size(), 0);
  EXPECT_EQ(recset.commitlogSize(), 0);

  cstable::CSTableReader reader(recset.getState().datafiles.back());
  void* data;
  size_t size;
  uint64_t r;
  uint64_t d;
  auto col = reader.getColumnReader("one");
  Set<String> res;
  for (int i = 0; i < reader.numRecords(); ++i) {
    col->next(&r, &d, &data, &size);
    res.emplace(String((char*) data, size));
  }

  EXPECT_EQ(res.count("1a"), 1);
  EXPECT_EQ(res.count("2a"), 1);
  EXPECT_EQ(res.count("3a"), 1);
  EXPECT_EQ(res.count("4a"), 1);
});

TEST_CASE(RecordSetTest, TestCommitlogReopen, [] () {
  auto schema = testSchema();
  RecordSet::RecordSetState state;

  {
    RecordSet recset(schema, "/tmp/__fnord_testrecset");
    recset.addRecord(0x42424242, testObject(schema, "1a", "1b"));
    recset.addRecord(0x23232323, testObject(schema, "2a", "2b"));
    state = recset.getState();
  }

  {
    RecordSet recset(schema, "/tmp/__fnord_testrecset", state);
    EXPECT_EQ(recset.getState().commitlog_size, state.commitlog_size);
    EXPECT_EQ(recset.getState().old_commitlogs.size(), 0);
    EXPECT_EQ(recset.commitlogSize(), 2);
  }
});

TEST_CASE(RecordSetTest, TestDuplicateRowsInCommitlog, [] () {
  auto schema = testSchema();
  RecordSet recset(schema, "/tmp/__fnord_testrecset");

  recset.addRecord(0x42424242, testObject(schema, "1a", "1b"));
  recset.addRecord(0x42424242, testObject(schema, "2a", "2b"));
  EXPECT_EQ(recset.commitlogSize(), 1);

  recset.rollCommitlog();
  EXPECT_EQ(recset.commitlogSize(), 1);

  recset.addRecord(0x42424242, testObject(schema, "3a", "3b"));
  recset.addRecord(0x32323232, testObject(schema, "2a", "2b"));
  EXPECT_EQ(recset.commitlogSize(), 2);

  recset.rollCommitlog();
  recset.compact();

  cstable::CSTableReader reader(recset.getState().datafiles.back());
  void* data;
  size_t size;
  uint64_t r;
  uint64_t d;
  auto col = reader.getColumnReader("__msgid");
  Set<uint64_t> res;
  for (int i = 0; i < reader.numRecords(); ++i) {
    col->next(&r, &d, &data, &size);
    res.emplace(*((uint64_t*) data));
  }

  EXPECT_EQ(res.size(), 2);
  EXPECT_EQ(res.count(0x42424242), 1);
  EXPECT_EQ(res.count(0x32323232), 1);
});

TEST_CASE(RecordSetTest, TestCompactionWithExistingTable, [] () {
  auto schema = testSchema();
  RecordSet recset(schema, "/tmp/__fnord_testrecset");

  recset.addRecord(0x42424242, testObject(schema, "1a", "1b"));
  recset.addRecord(0x23232323, testObject(schema, "2a", "2b"));
  recset.rollCommitlog();
  recset.compact();

  recset.addRecord(0x52525252, testObject(schema, "3a", "3b"));
  recset.addRecord(0x12121212, testObject(schema, "4a", "4b"));
  recset.rollCommitlog();
  recset.compact();

  auto msgids = recset.listRecords();

  EXPECT_EQ(msgids.size(), 4);
  EXPECT_EQ(msgids.count(0x42424242), 1);
  EXPECT_EQ(msgids.count(0x23232323), 1);
  EXPECT_EQ(msgids.count(0x52525252), 1);
  EXPECT_EQ(msgids.count(0x12121212), 1);
});

TEST_CASE(RecordSetTest, TestInsert10kRows, [] () {
  Random rnd;
  auto schema = testSchema();
  RecordSet recset(schema, "/tmp/__fnord_testrecset");

  for (int i = 0; i < 10; ++i) {
    for (int i = 0; i < 1000; ++i) {
      recset.addRecord(rnd.random64(), testObject(schema, "1a", "1b"));
    }

    recset.rollCommitlog();
  }

  EXPECT_EQ(recset.getState().old_commitlogs.size(), 10);
  recset.compact();
  EXPECT_EQ(recset.getState().datafiles.size(), 1);

  cstable::CSTableReader reader(recset.getState().datafiles.back());
  EXPECT_EQ(reader.numRecords(), 10000);
});



