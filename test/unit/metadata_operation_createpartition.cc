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
#include "eventql/db/metadata_store.h"
#include "eventql/db/metadata_operation.h"
#include "eventql/db/metadata_operations.pb.h"
#include "../unit_test.h"

namespace eventql {
namespace test {
namespace unit {

// UNIT-METADATAOPPCREATE-001
static bool test_metadata_operation_createpartition_create_empty() {
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
  EXPECT_EQ(output.size(), 1u);
  EXPECT_EQ(output[0].partition_id, SHA1::compute("X"));
  EXPECT_EQ(output[0].begin, encodePartitionKey(KEYSPACE_UINT64, "17"));
  EXPECT_EQ(output[0].end, encodePartitionKey(KEYSPACE_UINT64, "23"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[0].servers[0].server_id, "s1");
  EXPECT_EQ(output[0].servers[0].placement_id, 42u);
  EXPECT_EQ(output[0].servers[1].server_id, "s2");
  EXPECT_EQ(output[0].servers[1].placement_id, 42u);
  EXPECT_EQ(output[0].servers[2].server_id, "s3");
  EXPECT_EQ(output[0].servers[2].placement_id, 42u);

  return true;
}

// UNIT-METADATAOPPCREATE-002
static bool test_metadata_operation_createpartition_create_begin() {
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
  return true;
}

// UNIT-METADATAOPPCREATE-003
static bool test_metadata_operation_createpartition_create_begin2() {
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
  return true;
}

// UNIT-METADATAOPPCREATE-004
static bool test_metadata_operation_createpartition_create_middle() {
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

  return true;
}

// UNIT-METADATAOPPCREATE-005
static bool test_metadata_operation_createpartition_create_middle2() {
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
  return true;
}

// UNIT-METADATAOPPCREATE-006
static bool test_metadata_operation_createpartition_create_end() {
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

  return true;
}

// UNIT-METADATAOPPCREATE-007
static bool test_metadata_operation_createpartition_create_end2() {
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

  return true;
}

// UNIT-METADATAOPPCREATE-008
static bool test_metadata_operation_createpartition_create_overlap_begin() {
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

  return true;
}

// UNIT-METADATAOPPCREATE-009
static bool test_metadata_operation_createpartition_create_overlap_middle() {
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
  return true;
}

// UNIT-METADATAOPPCREATE-010
static bool test_metadata_operation_createpartition_create_overlap_end() {
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
  return true;
}

// UNIT-METADATAOPPCREATE-011
static bool test_metadata_operation_createpartition_create_overlap_exact() {
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

  return true;
}

void setup_unit_metadata_operation_createpartition_tests(TestRepository* repo) {
  std::vector<TestCase> c;
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-CREATEPART-001", metadata_operation_createpartition, create_empty);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-CREATEPART-002", metadata_operation_createpartition, create_begin);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-CREATEPART-003", metadata_operation_createpartition, create_begin2);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-CREATEPART-004", metadata_operation_createpartition, create_middle);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-CREATEPART-005", metadata_operation_createpartition, create_middle2);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-CREATEPART-006", metadata_operation_createpartition, create_end);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-CREATEPART-007", metadata_operation_createpartition, create_end2);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-CREATEPART-008", metadata_operation_createpartition, create_overlap_begin);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-CREATEPART-009", metadata_operation_createpartition, create_overlap_middle);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-CREATEPART-010", metadata_operation_createpartition, create_overlap_end);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-CREATEPART-011", metadata_operation_createpartition, create_overlap_exact);
  repo->addTestBundle(c);
}

} // namespace unit
} // namespace test
} // namespace eventql

