/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include "eventql/eventql.h"
#include "eventql/util/stdtypes.h"
#include "eventql/util/random.h"
#include "eventql/util/test/unittest.h"
#include "eventql/db/metadata_store.h"
#include "eventql/db/metadata_operation.h"
#include "eventql/db/metadata_operations.pb.h"

using namespace eventql;

UNIT_TEST(MetadataOperationPartitionCreate);

TEST_CASE(MetadataOperationPartitionCreate, TestCreateEmpty, [] () {
  Vector<MetadataFile::PartitionMapEntry> pmap;
  MetadataFile input(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_FINITE);

  auto new_partition_id = SHA1::compute("X");

  CreatePartitionOperation op;
  op.set_partition_id(new_partition_id.data(), new_partition_id.size());
  op.set_begin(encodePartitionKey(KEYSPACE_UINT64, "17"));
  op.set_end(encodePartitionKey(KEYSPACE_UINT64, "23"));
  op.set_placement_id(42);
  *op.add_servers() = "s1";
  *op.add_servers() = "s2";
  *op.add_servers() = "s3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_CREATE_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 1);
  EXPECT_EQ(output[0].partition_id, SHA1::compute("X"));
  EXPECT_EQ(output[0].begin, encodePartitionKey(KEYSPACE_UINT64, "17"));
  EXPECT_EQ(output[0].end, encodePartitionKey(KEYSPACE_UINT64, "23"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[0].servers[0].server_id, "s1");
  EXPECT_EQ(output[0].servers[0].placement_id, 42);
  EXPECT_EQ(output[0].servers[1].server_id, "s2");
  EXPECT_EQ(output[0].servers[1].placement_id, 42);
  EXPECT_EQ(output[0].servers[2].server_id, "s3");
  EXPECT_EQ(output[0].servers[2].placement_id, 42);
});

TEST_CASE(MetadataOperationPartitionCreate, TestCreateBegin, [] () {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "10");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "50");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "80");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "120");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "140");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "900");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_FINITE);

  auto new_partition_id = SHA1::compute("X");

  CreatePartitionOperation op;
  op.set_partition_id(new_partition_id.data(), new_partition_id.size());
  op.set_begin(encodePartitionKey(KEYSPACE_UINT64, "1"));
  op.set_end(encodePartitionKey(KEYSPACE_UINT64, "5"));
  op.set_placement_id(42);
  *op.add_servers() = "s1";
  *op.add_servers() = "s2";
  *op.add_servers() = "s3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_CREATE_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 5);
  EXPECT_EQ(output[0].partition_id, SHA1::compute("X"));
  EXPECT_EQ(output[0].begin, encodePartitionKey(KEYSPACE_UINT64, "1"));
  EXPECT_EQ(output[0].end, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[0].servers[0].server_id, "s1");
  EXPECT_EQ(output[0].servers[0].placement_id, 42);
  EXPECT_EQ(output[0].servers[1].server_id, "s2");
  EXPECT_EQ(output[0].servers[1].placement_id, 42);
  EXPECT_EQ(output[0].servers[2].server_id, "s3");
  EXPECT_EQ(output[0].servers[2].placement_id, 42);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "10"));
  EXPECT_EQ(output[1].end, encodePartitionKey(KEYSPACE_UINT64, "30"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "30"));
  EXPECT_EQ(output[2].end, encodePartitionKey(KEYSPACE_UINT64, "50"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "80"));
  EXPECT_EQ(output[3].end, encodePartitionKey(KEYSPACE_UINT64, "120"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[3].splitting, false);
  EXPECT_EQ(output[4].begin, encodePartitionKey(KEYSPACE_UINT64, "140"));
  EXPECT_EQ(output[4].end, encodePartitionKey(KEYSPACE_UINT64, "900"));
  EXPECT_EQ(output[4].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[4].splitting, false);
});

TEST_CASE(MetadataOperationPartitionCreate, TestCreateBegin2, [] () {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "10");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "50");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "80");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "120");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "140");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "900");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_FINITE);

  auto new_partition_id = SHA1::compute("X");

  CreatePartitionOperation op;
  op.set_partition_id(new_partition_id.data(), new_partition_id.size());
  op.set_begin(encodePartitionKey(KEYSPACE_UINT64, "0"));
  op.set_end(encodePartitionKey(KEYSPACE_UINT64, "10"));
  op.set_placement_id(42);
  *op.add_servers() = "s1";
  *op.add_servers() = "s2";
  *op.add_servers() = "s3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_CREATE_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 5);
  EXPECT_EQ(output[0].partition_id, SHA1::compute("X"));
  EXPECT_EQ(output[0].begin, encodePartitionKey(KEYSPACE_UINT64, "0"));
  EXPECT_EQ(output[0].end, encodePartitionKey(KEYSPACE_UINT64, "10"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[0].servers[0].server_id, "s1");
  EXPECT_EQ(output[0].servers[0].placement_id, 42);
  EXPECT_EQ(output[0].servers[1].server_id, "s2");
  EXPECT_EQ(output[0].servers[1].placement_id, 42);
  EXPECT_EQ(output[0].servers[2].server_id, "s3");
  EXPECT_EQ(output[0].servers[2].placement_id, 42);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "10"));
  EXPECT_EQ(output[1].end, encodePartitionKey(KEYSPACE_UINT64, "30"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "30"));
  EXPECT_EQ(output[2].end, encodePartitionKey(KEYSPACE_UINT64, "50"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "80"));
  EXPECT_EQ(output[3].end, encodePartitionKey(KEYSPACE_UINT64, "120"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[3].splitting, false);
  EXPECT_EQ(output[4].begin, encodePartitionKey(KEYSPACE_UINT64, "140"));
  EXPECT_EQ(output[4].end, encodePartitionKey(KEYSPACE_UINT64, "900"));
  EXPECT_EQ(output[4].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[4].splitting, false);
});

TEST_CASE(MetadataOperationPartitionCreate, TestCreateMiddle, [] () {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "10");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "50");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "80");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "120");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "140");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "900");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_FINITE);

  auto new_partition_id = SHA1::compute("X");

  CreatePartitionOperation op;
  op.set_partition_id(new_partition_id.data(), new_partition_id.size());
  op.set_begin(encodePartitionKey(KEYSPACE_UINT64, "60"));
  op.set_end(encodePartitionKey(KEYSPACE_UINT64, "70"));
  op.set_placement_id(42);
  *op.add_servers() = "s1";
  *op.add_servers() = "s2";
  *op.add_servers() = "s3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_CREATE_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 5);
  EXPECT_EQ(output[0].begin, encodePartitionKey(KEYSPACE_UINT64, "10"));
  EXPECT_EQ(output[0].end, encodePartitionKey(KEYSPACE_UINT64, "30"));
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "30"));
  EXPECT_EQ(output[1].end, encodePartitionKey(KEYSPACE_UINT64, "50"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].partition_id, SHA1::compute("X"));
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "60"));
  EXPECT_EQ(output[2].end, encodePartitionKey(KEYSPACE_UINT64, "70"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[2].servers[0].server_id, "s1");
  EXPECT_EQ(output[2].servers[0].placement_id, 42);
  EXPECT_EQ(output[2].servers[1].server_id, "s2");
  EXPECT_EQ(output[2].servers[1].placement_id, 42);
  EXPECT_EQ(output[2].servers[2].server_id, "s3");
  EXPECT_EQ(output[2].servers[2].placement_id, 42);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "80"));
  EXPECT_EQ(output[3].end, encodePartitionKey(KEYSPACE_UINT64, "120"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[3].splitting, false);
  EXPECT_EQ(output[4].begin, encodePartitionKey(KEYSPACE_UINT64, "140"));
  EXPECT_EQ(output[4].end, encodePartitionKey(KEYSPACE_UINT64, "900"));
  EXPECT_EQ(output[4].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[4].splitting, false);
});

TEST_CASE(MetadataOperationPartitionCreate, TestCreateMiddle2, [] () {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "10");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "50");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "80");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "120");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "140");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "900");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_FINITE);

  auto new_partition_id = SHA1::compute("X");

  CreatePartitionOperation op;
  op.set_partition_id(new_partition_id.data(), new_partition_id.size());
  op.set_begin(encodePartitionKey(KEYSPACE_UINT64, "50"));
  op.set_end(encodePartitionKey(KEYSPACE_UINT64, "80"));
  op.set_placement_id(42);
  *op.add_servers() = "s1";
  *op.add_servers() = "s2";
  *op.add_servers() = "s3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_CREATE_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 5);
  EXPECT_EQ(output[0].begin, encodePartitionKey(KEYSPACE_UINT64, "10"));
  EXPECT_EQ(output[0].end, encodePartitionKey(KEYSPACE_UINT64, "30"));
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "30"));
  EXPECT_EQ(output[1].end, encodePartitionKey(KEYSPACE_UINT64, "50"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].partition_id, SHA1::compute("X"));
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "50"));
  EXPECT_EQ(output[2].end, encodePartitionKey(KEYSPACE_UINT64, "80"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[2].servers[0].server_id, "s1");
  EXPECT_EQ(output[2].servers[0].placement_id, 42);
  EXPECT_EQ(output[2].servers[1].server_id, "s2");
  EXPECT_EQ(output[2].servers[1].placement_id, 42);
  EXPECT_EQ(output[2].servers[2].server_id, "s3");
  EXPECT_EQ(output[2].servers[2].placement_id, 42);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "80"));
  EXPECT_EQ(output[3].end, encodePartitionKey(KEYSPACE_UINT64, "120"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[3].splitting, false);
  EXPECT_EQ(output[4].begin, encodePartitionKey(KEYSPACE_UINT64, "140"));
  EXPECT_EQ(output[4].end, encodePartitionKey(KEYSPACE_UINT64, "900"));
  EXPECT_EQ(output[4].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[4].splitting, false);
});

TEST_CASE(MetadataOperationPartitionCreate, TestCreateEnd, [] () {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "10");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "50");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "80");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "120");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "140");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "900");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_FINITE);

  auto new_partition_id = SHA1::compute("X");

  CreatePartitionOperation op;
  op.set_partition_id(new_partition_id.data(), new_partition_id.size());
  op.set_begin(encodePartitionKey(KEYSPACE_UINT64, "1000"));
  op.set_end(encodePartitionKey(KEYSPACE_UINT64, "1500"));
  op.set_placement_id(42);
  *op.add_servers() = "s1";
  *op.add_servers() = "s2";
  *op.add_servers() = "s3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_CREATE_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 5);
  EXPECT_EQ(output[0].begin, encodePartitionKey(KEYSPACE_UINT64, "10"));
  EXPECT_EQ(output[0].end, encodePartitionKey(KEYSPACE_UINT64, "30"));
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "30"));
  EXPECT_EQ(output[1].end, encodePartitionKey(KEYSPACE_UINT64, "50"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "80"));
  EXPECT_EQ(output[2].end, encodePartitionKey(KEYSPACE_UINT64, "120"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "140"));
  EXPECT_EQ(output[3].end, encodePartitionKey(KEYSPACE_UINT64, "900"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[3].splitting, false);
  EXPECT_EQ(output[4].partition_id, SHA1::compute("X"));
  EXPECT_EQ(output[4].begin, encodePartitionKey(KEYSPACE_UINT64, "1000"));
  EXPECT_EQ(output[4].end, encodePartitionKey(KEYSPACE_UINT64, "1500"));
  EXPECT_EQ(output[4].splitting, false);
  EXPECT_EQ(output[4].servers[0].server_id, "s1");
  EXPECT_EQ(output[4].servers[0].placement_id, 42);
  EXPECT_EQ(output[4].servers[1].server_id, "s2");
  EXPECT_EQ(output[4].servers[1].placement_id, 42);
  EXPECT_EQ(output[4].servers[2].server_id, "s3");
  EXPECT_EQ(output[4].servers[2].placement_id, 42);
});

TEST_CASE(MetadataOperationPartitionCreate, TestCreateEnd2, [] () {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "10");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "50");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "80");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "120");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "140");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "900");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_FINITE);

  auto new_partition_id = SHA1::compute("X");

  CreatePartitionOperation op;
  op.set_partition_id(new_partition_id.data(), new_partition_id.size());
  op.set_begin(encodePartitionKey(KEYSPACE_UINT64, "900"));
  op.set_end(encodePartitionKey(KEYSPACE_UINT64, "1500"));
  op.set_placement_id(42);
  *op.add_servers() = "s1";
  *op.add_servers() = "s2";
  *op.add_servers() = "s3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_CREATE_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 5);
  EXPECT_EQ(output[0].begin, encodePartitionKey(KEYSPACE_UINT64, "10"));
  EXPECT_EQ(output[0].end, encodePartitionKey(KEYSPACE_UINT64, "30"));
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "30"));
  EXPECT_EQ(output[1].end, encodePartitionKey(KEYSPACE_UINT64, "50"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "80"));
  EXPECT_EQ(output[2].end, encodePartitionKey(KEYSPACE_UINT64, "120"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "140"));
  EXPECT_EQ(output[3].end, encodePartitionKey(KEYSPACE_UINT64, "900"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[3].splitting, false);
  EXPECT_EQ(output[4].partition_id, SHA1::compute("X"));
  EXPECT_EQ(output[4].begin, encodePartitionKey(KEYSPACE_UINT64, "900"));
  EXPECT_EQ(output[4].end, encodePartitionKey(KEYSPACE_UINT64, "1500"));
  EXPECT_EQ(output[4].splitting, false);
  EXPECT_EQ(output[4].servers[0].server_id, "s1");
  EXPECT_EQ(output[4].servers[0].placement_id, 42);
  EXPECT_EQ(output[4].servers[1].server_id, "s2");
  EXPECT_EQ(output[4].servers[1].placement_id, 42);
  EXPECT_EQ(output[4].servers[2].server_id, "s3");
  EXPECT_EQ(output[4].servers[2].placement_id, 42);
});

TEST_CASE(MetadataOperationPartitionCreate, TestCreateOverlapBegin, [] () {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "10");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "50");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "80");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "120");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "140");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "900");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_FINITE);

  auto new_partition_id = SHA1::compute("X");

  CreatePartitionOperation op;
  op.set_partition_id(new_partition_id.data(), new_partition_id.size());
  op.set_begin(encodePartitionKey(KEYSPACE_UINT64, "1"));
  op.set_end(encodePartitionKey(KEYSPACE_UINT64, "15"));
  op.set_placement_id(42);
  *op.add_servers() = "s1";
  *op.add_servers() = "s2";
  *op.add_servers() = "s3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_CREATE_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(!rc.isSuccess());
});

TEST_CASE(MetadataOperationPartitionCreate, TestCreateOverlapMiddle, [] () {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "10");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "50");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "80");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "120");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "140");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "900");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_FINITE);

  auto new_partition_id = SHA1::compute("X");

  CreatePartitionOperation op;
  op.set_partition_id(new_partition_id.data(), new_partition_id.size());
  op.set_begin(encodePartitionKey(KEYSPACE_UINT64, "60"));
  op.set_end(encodePartitionKey(KEYSPACE_UINT64, "90"));
  op.set_placement_id(42);
  *op.add_servers() = "s1";
  *op.add_servers() = "s2";
  *op.add_servers() = "s3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_CREATE_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(!rc.isSuccess());
});

TEST_CASE(MetadataOperationPartitionCreate, TestCreateOverlapEnd, [] () {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "10");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "50");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "80");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "120");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "140");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "900");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_FINITE);

  auto new_partition_id = SHA1::compute("X");

  CreatePartitionOperation op;
  op.set_partition_id(new_partition_id.data(), new_partition_id.size());
  op.set_begin(encodePartitionKey(KEYSPACE_UINT64, "899"));
  op.set_end(encodePartitionKey(KEYSPACE_UINT64, "910"));
  op.set_placement_id(42);
  *op.add_servers() = "s1";
  *op.add_servers() = "s2";
  *op.add_servers() = "s3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_CREATE_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(!rc.isSuccess());
});

TEST_CASE(MetadataOperationPartitionCreate, TestCreateOverlapExact, [] () {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "10");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "30");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "50");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "80");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "120");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "140");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "900");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_FINITE);

  auto new_partition_id = SHA1::compute("X");

  CreatePartitionOperation op;
  op.set_partition_id(new_partition_id.data(), new_partition_id.size());
  op.set_begin(encodePartitionKey(KEYSPACE_UINT64, "80"));
  op.set_end(encodePartitionKey(KEYSPACE_UINT64, "120"));
  op.set_placement_id(42);
  *op.add_servers() = "s1";
  *op.add_servers() = "s2";
  *op.add_servers() = "s3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_CREATE_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(!rc.isSuccess());
});

