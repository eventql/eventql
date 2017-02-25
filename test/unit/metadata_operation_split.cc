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

static MetadataFile::PartitionPlacement mkPlacement(
    const String& server_id,
    uint64_t placement_id) {
  MetadataFile::PartitionPlacement p;
  p.server_id = server_id;
  p.placement_id = placement_id;
  return p;
}

// UNIT-METAOP-SPLIT-001
static bool test_metadata_operation_split_split_begin() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(SHA1::compute("mytx"), 0, KEYSPACE_UINT64, pmap, 0);

  auto split_partition_id = SHA1::compute("A");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "2"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(23);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 4);
  EXPECT_EQ(output[0].begin, "");
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, true);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[3].splitting, false);

  MetadataFile input2(SHA1::compute("mytx2"), 0, KEYSPACE_UINT64, output, 0);

  FinalizeSplitOperation op2;
  op2.set_partition_id(split_partition_id.data(), split_partition_id.size());

  std::string opdata_buf2;
  op2.SerializeToString(&opdata_buf2);

  MetadataOperation openv2(
      "test",
      "test",
      METAOP_FINALIZE_SPLIT,
      SHA1::compute("mytx2"),
      SHA1::compute("mytxout2"),
      Buffer(opdata_buf2));

  Vector<MetadataFile::PartitionMapEntry> output2;
  rc = openv2.perform(input2, &output2);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output2.size(), 5);
  EXPECT_EQ(output2[0].begin, "");
  EXPECT_EQ(output2[0].partition_id, SHA1::compute("X1"));
  EXPECT_EQ(output2[0].splitting, false);
  EXPECT_EQ(output2[0].servers.size(), 3);
  EXPECT_EQ(output2[0].servers[0].server_id, "l1");
  EXPECT_EQ(output2[0].servers[0].placement_id, 23);
  EXPECT_EQ(output2[0].servers[1].server_id, "l2");
  EXPECT_EQ(output2[0].servers[1].placement_id, 23);
  EXPECT_EQ(output2[0].servers[2].server_id, "l3");
  EXPECT_EQ(output2[0].servers[2].placement_id, 23);
  EXPECT_EQ(output2[1].begin, encodePartitionKey(KEYSPACE_UINT64, "2"));
  EXPECT_EQ(output2[1].partition_id, SHA1::compute("X2"));
  EXPECT_EQ(output2[1].splitting, false);
  EXPECT_EQ(output2[1].servers.size(), 3);
  EXPECT_EQ(output2[1].servers[0].server_id, "h1");
  EXPECT_EQ(output2[1].servers[0].placement_id, 23);
  EXPECT_EQ(output2[1].servers[1].server_id, "h2");
  EXPECT_EQ(output2[1].servers[1].placement_id, 23);
  EXPECT_EQ(output2[1].servers[2].server_id, "h3");
  EXPECT_EQ(output2[1].servers[2].placement_id, 23);
  EXPECT_EQ(output2[2].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output2[2].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output2[2].splitting, false);
  EXPECT_EQ(output2[3].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output2[3].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output2[3].splitting, false);
  EXPECT_EQ(output2[4].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output2[4].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output2[4].splitting, false);
  return true;
}

// unit-metaop-split-002
static bool test_metadata_operation_split_subsplit_begin_low() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.partition_id = SHA1::compute("B");
    e.splitting = true;
    e.split_point = encodePartitionKey(KEYSPACE_UINT64, "5");
    e.split_partition_id_low = SHA1::compute("lowlow");
    e.split_partition_id_high = SHA1::compute("highhigh");
    e.split_servers_low.emplace_back(mkPlacement("s8", 43));
    e.split_servers_low.emplace_back(mkPlacement("s9", 41));
    e.split_servers_low.emplace_back(mkPlacement("s6", 42));
    e.split_servers_high.emplace_back(mkPlacement("s5", 51));
    e.split_servers_high.emplace_back(mkPlacement("s7", 52));
    e.split_servers_high.emplace_back(mkPlacement("s3", 53));
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(SHA1::compute("mytx"), 0, KEYSPACE_UINT64, pmap, 0);

  auto split_partition_id = SHA1::compute("lowlow");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "4"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(17);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 4);
  EXPECT_EQ(output[0].partition_id, SHA1::compute("lowlow"));
  EXPECT_EQ(output[0].begin, "");
  EXPECT_EQ(output[0].splitting, true);
  EXPECT_EQ(output[0].split_point, encodePartitionKey(KEYSPACE_UINT64, "4"));
  EXPECT_EQ(output[0].split_partition_id_low, SHA1::compute("X1"));
  EXPECT_EQ(output[0].split_servers_low[0].server_id, "l1");
  EXPECT_EQ(output[0].split_servers_low[0].placement_id, 17);
  EXPECT_EQ(output[0].split_servers_low[1].server_id, "l2");
  EXPECT_EQ(output[0].split_servers_low[1].placement_id, 17);
  EXPECT_EQ(output[0].split_servers_low[2].server_id, "l3");
  EXPECT_EQ(output[0].split_servers_low[2].placement_id, 17);
  EXPECT_EQ(output[0].split_partition_id_high, SHA1::compute("X2"));
  EXPECT_EQ(output[0].split_servers_high[0].server_id, "h1");
  EXPECT_EQ(output[0].split_servers_high[0].placement_id, 17);
  EXPECT_EQ(output[0].split_servers_high[1].server_id, "h2");
  EXPECT_EQ(output[0].split_servers_high[1].placement_id, 17);
  EXPECT_EQ(output[0].split_servers_high[2].server_id, "h3");
  EXPECT_EQ(output[0].split_servers_high[2].placement_id, 17);
  EXPECT_EQ(output[1].partition_id, SHA1::compute("highhigh"));
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[3].splitting, false);
  return true;
}

// UNIT-METAOP-SPLIT-003
static bool test_metadata_operation_split_subsplit_begin_high() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.partition_id = SHA1::compute("B");
    e.splitting = true;
    e.split_point = encodePartitionKey(KEYSPACE_UINT64, "5");
    e.split_partition_id_low = SHA1::compute("lowlow");
    e.split_partition_id_high = SHA1::compute("highhigh");
    e.split_servers_low.emplace_back(mkPlacement("s8", 43));
    e.split_servers_low.emplace_back(mkPlacement("s9", 41));
    e.split_servers_low.emplace_back(mkPlacement("s6", 42));
    e.split_servers_high.emplace_back(mkPlacement("s5", 51));
    e.split_servers_high.emplace_back(mkPlacement("s7", 52));
    e.split_servers_high.emplace_back(mkPlacement("s3", 53));
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(SHA1::compute("mytx"), 0, KEYSPACE_UINT64, pmap, 0);

  auto split_partition_id = SHA1::compute("highhigh");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "7"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(17);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 4);
  EXPECT_EQ(output[0].partition_id, SHA1::compute("lowlow"));
  EXPECT_EQ(output[0].begin, "");
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].partition_id, SHA1::compute("highhigh"));
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output[1].splitting, true);
  EXPECT_EQ(output[1].split_point, encodePartitionKey(KEYSPACE_UINT64, "7"));
  EXPECT_EQ(output[1].split_partition_id_low, SHA1::compute("X1"));
  EXPECT_EQ(output[1].split_servers_low[0].server_id, "l1");
  EXPECT_EQ(output[1].split_servers_low[0].placement_id, 17);
  EXPECT_EQ(output[1].split_servers_low[1].server_id, "l2");
  EXPECT_EQ(output[1].split_servers_low[1].placement_id, 17);
  EXPECT_EQ(output[1].split_servers_low[2].server_id, "l3");
  EXPECT_EQ(output[1].split_servers_low[2].placement_id, 17);
  EXPECT_EQ(output[1].split_partition_id_high, SHA1::compute("X2"));
  EXPECT_EQ(output[1].split_servers_high[0].server_id, "h1");
  EXPECT_EQ(output[1].split_servers_high[0].placement_id, 17);
  EXPECT_EQ(output[1].split_servers_high[1].server_id, "h2");
  EXPECT_EQ(output[1].split_servers_high[1].placement_id, 17);
  EXPECT_EQ(output[1].split_servers_high[2].server_id, "h3");
  EXPECT_EQ(output[1].split_servers_high[2].placement_id, 17);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[3].splitting, false);
  return true;
}

// UNIT-METAOP-SPLIT-004
static bool test_metadata_operation_split_split_middle() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(SHA1::compute("mytx"), 0, KEYSPACE_UINT64, pmap, 0);

  auto split_partition_id = SHA1::compute("C");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "10"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(42);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 4);
  EXPECT_EQ(output[0].begin, "");
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[2].splitting, true);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[3].splitting, false);

  MetadataFile input2(SHA1::compute("mytx2"), 0, KEYSPACE_UINT64, output, 0);

  FinalizeSplitOperation op2;
  op2.set_partition_id(split_partition_id.data(), split_partition_id.size());

  std::string opdata_buf2;
  op2.SerializeToString(&opdata_buf2);

  MetadataOperation openv2(
      "test",
      "test",
      METAOP_FINALIZE_SPLIT,
      SHA1::compute("mytx2"),
      SHA1::compute("mytxout2"),
      Buffer(opdata_buf2));

  Vector<MetadataFile::PartitionMapEntry> output2;
  rc = openv2.perform(input2, &output2);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output2.size(), 5);
  EXPECT_EQ(output2[0].begin, "");
  EXPECT_EQ(output2[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output2[0].splitting, false);
  EXPECT_EQ(output2[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output2[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output2[1].splitting, false);
  EXPECT_EQ(output2[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output2[2].partition_id, SHA1::compute("X1"));
  EXPECT_EQ(output2[2].splitting, false);
  EXPECT_EQ(output2[2].servers.size(), 3);
  EXPECT_EQ(output2[2].servers[0].server_id, "l1");
  EXPECT_EQ(output2[2].servers[0].placement_id, 42);
  EXPECT_EQ(output2[2].servers[1].server_id, "l2");
  EXPECT_EQ(output2[2].servers[1].placement_id, 42);
  EXPECT_EQ(output2[2].servers[2].server_id, "l3");
  EXPECT_EQ(output2[2].servers[2].placement_id, 42);
  EXPECT_EQ(output2[3].begin, encodePartitionKey(KEYSPACE_UINT64, "10"));
  EXPECT_EQ(output2[3].partition_id, SHA1::compute("X2"));
  EXPECT_EQ(output2[3].splitting, false);
  EXPECT_EQ(output2[3].servers.size(), 3);
  EXPECT_EQ(output2[3].servers[0].server_id, "h1");
  EXPECT_EQ(output2[3].servers[0].placement_id, 42);
  EXPECT_EQ(output2[3].servers[1].server_id, "h2");
  EXPECT_EQ(output2[3].servers[1].placement_id, 42);
  EXPECT_EQ(output2[3].servers[2].server_id, "h3");
  EXPECT_EQ(output2[3].servers[2].placement_id, 42);
  EXPECT_EQ(output2[4].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output2[4].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output2[4].splitting, false);
  return true;
}

// UNIT-METAOP-SPLIT-005
static bool test_metadata_operation_split_subsplit_middle_low() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("B");
    e.splitting = true;
    e.split_point = encodePartitionKey(KEYSPACE_UINT64, "5");
    e.split_partition_id_low = SHA1::compute("lowlow");
    e.split_partition_id_high = SHA1::compute("highhigh");
    e.split_servers_low.emplace_back(mkPlacement("s8", 43));
    e.split_servers_low.emplace_back(mkPlacement("s9", 41));
    e.split_servers_low.emplace_back(mkPlacement("s6", 42));
    e.split_servers_high.emplace_back(mkPlacement("s5", 51));
    e.split_servers_high.emplace_back(mkPlacement("s7", 52));
    e.split_servers_high.emplace_back(mkPlacement("s3", 53));
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(SHA1::compute("mytx"), 0, KEYSPACE_UINT64, pmap, 0);

  auto split_partition_id = SHA1::compute("lowlow");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "4"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(17);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 5);
  EXPECT_EQ(output[0].begin, "");
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].partition_id, SHA1::compute("lowlow"));
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[1].splitting, true);
  EXPECT_EQ(output[1].split_point, encodePartitionKey(KEYSPACE_UINT64, "4"));
  EXPECT_EQ(output[1].split_partition_id_low, SHA1::compute("X1"));
  EXPECT_EQ(output[1].split_servers_low[0].server_id, "l1");
  EXPECT_EQ(output[1].split_servers_low[0].placement_id, 17);
  EXPECT_EQ(output[1].split_servers_low[1].server_id, "l2");
  EXPECT_EQ(output[1].split_servers_low[1].placement_id, 17);
  EXPECT_EQ(output[1].split_servers_low[2].server_id, "l3");
  EXPECT_EQ(output[1].split_servers_low[2].placement_id, 17);
  EXPECT_EQ(output[1].split_partition_id_high, SHA1::compute("X2"));
  EXPECT_EQ(output[1].split_servers_high[0].server_id, "h1");
  EXPECT_EQ(output[1].split_servers_high[0].placement_id, 17);
  EXPECT_EQ(output[1].split_servers_high[1].server_id, "h2");
  EXPECT_EQ(output[1].split_servers_high[1].placement_id, 17);
  EXPECT_EQ(output[1].split_servers_high[2].server_id, "h3");
  EXPECT_EQ(output[1].split_servers_high[2].placement_id, 17);
  EXPECT_EQ(output[2].partition_id, SHA1::compute("highhigh"));
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[3].splitting, false);
  EXPECT_EQ(output[4].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[4].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[4].splitting, false);
  return true;
}

// UNIT-METAOP-SPLIT-006
static bool test_metadata_operation_split_subsplit_middle_high() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("B");
    e.splitting = true;
    e.split_point = encodePartitionKey(KEYSPACE_UINT64, "5");
    e.split_partition_id_low = SHA1::compute("lowlow");
    e.split_partition_id_high = SHA1::compute("highhigh");
    e.split_servers_low.emplace_back(mkPlacement("s8", 43));
    e.split_servers_low.emplace_back(mkPlacement("s9", 41));
    e.split_servers_low.emplace_back(mkPlacement("s6", 42));
    e.split_servers_high.emplace_back(mkPlacement("s5", 51));
    e.split_servers_high.emplace_back(mkPlacement("s7", 52));
    e.split_servers_high.emplace_back(mkPlacement("s3", 53));
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(SHA1::compute("mytx"), 0, KEYSPACE_UINT64, pmap, 0);

  auto split_partition_id = SHA1::compute("highhigh");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "7"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(17);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 5);
  EXPECT_EQ(output[0].begin, "");
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].partition_id, SHA1::compute("lowlow"));
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].partition_id, SHA1::compute("highhigh"));
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output[2].splitting, true);
  EXPECT_EQ(output[2].split_point, encodePartitionKey(KEYSPACE_UINT64, "7"));
  EXPECT_EQ(output[2].split_partition_id_low, SHA1::compute("X1"));
  EXPECT_EQ(output[2].split_servers_low[0].server_id, "l1");
  EXPECT_EQ(output[2].split_servers_low[0].placement_id, 17);
  EXPECT_EQ(output[2].split_servers_low[1].server_id, "l2");
  EXPECT_EQ(output[2].split_servers_low[1].placement_id, 17);
  EXPECT_EQ(output[2].split_servers_low[2].server_id, "l3");
  EXPECT_EQ(output[2].split_servers_low[2].placement_id, 17);
  EXPECT_EQ(output[2].split_partition_id_high, SHA1::compute("X2"));
  EXPECT_EQ(output[2].split_servers_high[0].server_id, "h1");
  EXPECT_EQ(output[2].split_servers_high[0].placement_id, 17);
  EXPECT_EQ(output[2].split_servers_high[1].server_id, "h2");
  EXPECT_EQ(output[2].split_servers_high[1].placement_id, 17);
  EXPECT_EQ(output[2].split_servers_high[2].server_id, "h3");
  EXPECT_EQ(output[2].split_servers_high[2].placement_id, 17);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[3].splitting, false);
  EXPECT_EQ(output[4].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[4].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[4].splitting, false);
  return true;
}

// UNIT-METAOP-SPLIT-007
static bool test_metadata_operation_split_split_end() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(SHA1::compute("mytx"), 0, KEYSPACE_UINT64, pmap, 0);

  auto split_partition_id = SHA1::compute("D");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "23"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(17);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 4);
  EXPECT_EQ(output[0].begin, "");
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[3].splitting, true);

  MetadataFile input2(SHA1::compute("mytx2"), 0, KEYSPACE_UINT64, output, 0);

  FinalizeSplitOperation op2;
  op2.set_partition_id(split_partition_id.data(), split_partition_id.size());

  std::string opdata_buf2;
  op2.SerializeToString(&opdata_buf2);

  MetadataOperation openv2(
      "test",
      "test",
      METAOP_FINALIZE_SPLIT,
      SHA1::compute("mytx2"),
      SHA1::compute("mytxout2"),
      Buffer(opdata_buf2));

  Vector<MetadataFile::PartitionMapEntry> output2;
  rc = openv2.perform(input2, &output2);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output2.size(), 5);
  EXPECT_EQ(output2[0].begin, "");
  EXPECT_EQ(output2[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output2[0].splitting, false);
  EXPECT_EQ(output2[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output2[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output2[1].splitting, false);
  EXPECT_EQ(output2[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output2[2].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output2[2].splitting, false);
  EXPECT_EQ(output2[3].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output2[3].partition_id, SHA1::compute("X1"));
  EXPECT_EQ(output2[3].splitting, false);
  EXPECT_EQ(output2[3].servers.size(), 3);
  EXPECT_EQ(output2[3].servers[0].server_id, "l1");
  EXPECT_EQ(output2[3].servers[0].placement_id, 17);
  EXPECT_EQ(output2[3].servers[1].server_id, "l2");
  EXPECT_EQ(output2[3].servers[1].placement_id, 17);
  EXPECT_EQ(output2[3].servers[2].server_id, "l3");
  EXPECT_EQ(output2[3].servers[2].placement_id, 17);
  EXPECT_EQ(output2[4].begin, encodePartitionKey(KEYSPACE_UINT64, "23"));
  EXPECT_EQ(output2[4].partition_id, SHA1::compute("X2"));
  EXPECT_EQ(output2[4].splitting, false);
  EXPECT_EQ(output2[4].servers.size(), 3);
  EXPECT_EQ(output2[4].servers[0].server_id, "h1");
  EXPECT_EQ(output2[4].servers[0].placement_id, 17);
  EXPECT_EQ(output2[4].servers[1].server_id, "h2");
  EXPECT_EQ(output2[4].servers[1].placement_id, 17);
  EXPECT_EQ(output2[4].servers[2].server_id, "h3");
  EXPECT_EQ(output2[4].servers[2].placement_id, 17);
  return true;
}

// UNIT-METAOP-SPLIT-008
static bool test_metadata_operation_split_subsplit_end_low() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "1");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("B");
    e.splitting = true;
    e.split_point = encodePartitionKey(KEYSPACE_UINT64, "5");
    e.split_partition_id_low = SHA1::compute("lowlow");
    e.split_partition_id_high = SHA1::compute("highhigh");
    e.split_servers_low.emplace_back(mkPlacement("s8", 43));
    e.split_servers_low.emplace_back(mkPlacement("s9", 41));
    e.split_servers_low.emplace_back(mkPlacement("s6", 42));
    e.split_servers_high.emplace_back(mkPlacement("s5", 51));
    e.split_servers_high.emplace_back(mkPlacement("s7", 52));
    e.split_servers_high.emplace_back(mkPlacement("s3", 53));
    pmap.emplace_back(e);
  }

  MetadataFile input(SHA1::compute("mytx"), 0, KEYSPACE_UINT64, pmap, 0);

  auto split_partition_id = SHA1::compute("lowlow");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "4"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(17);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 4);
  EXPECT_EQ(output[0].begin, "");
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "1"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].partition_id, SHA1::compute("lowlow"));
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[2].splitting, true);
  EXPECT_EQ(output[2].split_point, encodePartitionKey(KEYSPACE_UINT64, "4"));
  EXPECT_EQ(output[2].split_partition_id_low, SHA1::compute("X1"));
  EXPECT_EQ(output[2].split_servers_low[0].server_id, "l1");
  EXPECT_EQ(output[2].split_servers_low[0].placement_id, 17);
  EXPECT_EQ(output[2].split_servers_low[1].server_id, "l2");
  EXPECT_EQ(output[2].split_servers_low[1].placement_id, 17);
  EXPECT_EQ(output[2].split_servers_low[2].server_id, "l3");
  EXPECT_EQ(output[2].split_servers_low[2].placement_id, 17);
  EXPECT_EQ(output[2].split_partition_id_high, SHA1::compute("X2"));
  EXPECT_EQ(output[2].split_servers_high[0].server_id, "h1");
  EXPECT_EQ(output[2].split_servers_high[0].placement_id, 17);
  EXPECT_EQ(output[2].split_servers_high[1].server_id, "h2");
  EXPECT_EQ(output[2].split_servers_high[1].placement_id, 17);
  EXPECT_EQ(output[2].split_servers_high[2].server_id, "h3");
  EXPECT_EQ(output[2].split_servers_high[2].placement_id, 17);
  EXPECT_EQ(output[3].partition_id, SHA1::compute("highhigh"));
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output[3].splitting, false);
  return true;
}

// UNIT-METAOP-SPLIT-009
static bool test_metadata_operation_split_subsplit_end_high() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "1");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("B");
    e.splitting = true;
    e.split_point = encodePartitionKey(KEYSPACE_UINT64, "5");
    e.split_partition_id_low = SHA1::compute("lowlow");
    e.split_partition_id_high = SHA1::compute("highhigh");
    e.split_servers_low.emplace_back(mkPlacement("s8", 43));
    e.split_servers_low.emplace_back(mkPlacement("s9", 41));
    e.split_servers_low.emplace_back(mkPlacement("s6", 42));
    e.split_servers_high.emplace_back(mkPlacement("s5", 51));
    e.split_servers_high.emplace_back(mkPlacement("s7", 52));
    e.split_servers_high.emplace_back(mkPlacement("s3", 53));
    pmap.emplace_back(e);
  }

  MetadataFile input(SHA1::compute("mytx"), 0, KEYSPACE_UINT64, pmap, 0);

  auto split_partition_id = SHA1::compute("highhigh");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "7"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(17);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 4);
  EXPECT_EQ(output[0].begin, "");
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "1"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].partition_id, SHA1::compute("lowlow"));
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].partition_id, SHA1::compute("highhigh"));
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output[3].splitting, true);
  EXPECT_EQ(output[3].split_point, encodePartitionKey(KEYSPACE_UINT64, "7"));
  EXPECT_EQ(output[3].split_partition_id_low, SHA1::compute("X1"));
  EXPECT_EQ(output[3].split_servers_low[0].server_id, "l1");
  EXPECT_EQ(output[3].split_servers_low[0].placement_id, 17);
  EXPECT_EQ(output[3].split_servers_low[1].server_id, "l2");
  EXPECT_EQ(output[3].split_servers_low[1].placement_id, 17);
  EXPECT_EQ(output[3].split_servers_low[2].server_id, "l3");
  EXPECT_EQ(output[3].split_servers_low[2].placement_id, 17);
  EXPECT_EQ(output[3].split_partition_id_high, SHA1::compute("X2"));
  EXPECT_EQ(output[3].split_servers_high[0].server_id, "h1");
  EXPECT_EQ(output[3].split_servers_high[0].placement_id, 17);
  EXPECT_EQ(output[3].split_servers_high[1].server_id, "h2");
  EXPECT_EQ(output[3].split_servers_high[1].placement_id, 17);
  EXPECT_EQ(output[3].split_servers_high[2].server_id, "h3");
  EXPECT_EQ(output[3].split_servers_high[2].placement_id, 17);
  return true;
}

// UNIT-METAOP-SPLIT-010
static bool test_metadata_operation_split_finite_split_begin() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "1");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "5");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "12");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "99");
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

  auto split_partition_id = SHA1::compute("A");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "2"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(42);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 4);
  EXPECT_EQ(output[0].begin, encodePartitionKey(KEYSPACE_UINT64, "1"));
  EXPECT_EQ(output[0].end, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, true);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[1].end, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[2].end, encodePartitionKey(KEYSPACE_UINT64, "12"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[3].end, encodePartitionKey(KEYSPACE_UINT64, "99"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[3].splitting, false);

  MetadataFile input2(
      SHA1::compute("mytx2"),
      0,
      KEYSPACE_UINT64,
      output,
      MFILE_FINITE);

  FinalizeSplitOperation op2;
  op2.set_partition_id(split_partition_id.data(), split_partition_id.size());

  std::string opdata_buf2;
  op2.SerializeToString(&opdata_buf2);

  MetadataOperation openv2(
      "test",
      "test",
      METAOP_FINALIZE_SPLIT,
      SHA1::compute("mytx2"),
      SHA1::compute("mytxout2"),
      Buffer(opdata_buf2));

  EXPECT(input2.hasFinitePartitions() == true);

  Vector<MetadataFile::PartitionMapEntry> output2;
  rc = openv2.perform(input2, &output2);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output2.size(), 5);
  EXPECT_EQ(output2[0].begin, encodePartitionKey(KEYSPACE_UINT64, "1"));
  EXPECT_EQ(output2[0].end, encodePartitionKey(KEYSPACE_UINT64, "2"));
  EXPECT_EQ(output2[0].partition_id, SHA1::compute("X1"));
  EXPECT_EQ(output2[0].splitting, false);
  EXPECT_EQ(output2[0].servers.size(), 3);
  EXPECT_EQ(output2[0].servers[0].server_id, "l1");
  EXPECT_EQ(output2[0].servers[0].placement_id, 42);
  EXPECT_EQ(output2[0].servers[1].server_id, "l2");
  EXPECT_EQ(output2[0].servers[1].placement_id, 42);
  EXPECT_EQ(output2[0].servers[2].server_id, "l3");
  EXPECT_EQ(output2[0].servers[2].placement_id, 42);
  EXPECT_EQ(output2[1].begin, encodePartitionKey(KEYSPACE_UINT64, "2"));
  EXPECT_EQ(output2[1].end, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output2[1].partition_id, SHA1::compute("X2"));
  EXPECT_EQ(output2[1].splitting, false);
  EXPECT_EQ(output2[1].servers.size(), 3);
  EXPECT_EQ(output2[1].servers[0].server_id, "h1");
  EXPECT_EQ(output2[1].servers[0].placement_id, 42);
  EXPECT_EQ(output2[1].servers[1].server_id, "h2");
  EXPECT_EQ(output2[1].servers[1].placement_id, 42);
  EXPECT_EQ(output2[1].servers[2].server_id, "h3");
  EXPECT_EQ(output2[1].servers[2].placement_id, 42);
  EXPECT_EQ(output2[2].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output2[2].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output2[2].end, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output2[2].splitting, false);
  EXPECT_EQ(output2[3].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output2[3].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output2[3].end, encodePartitionKey(KEYSPACE_UINT64, "12"));
  EXPECT_EQ(output2[3].splitting, false);
  EXPECT_EQ(output2[4].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output2[4].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output2[4].end, encodePartitionKey(KEYSPACE_UINT64, "99"));
  EXPECT_EQ(output2[4].splitting, false);
  return true;
}

// UNIT-METAOP-SPLIT-011
static bool test_metadata_operation_split_finite_split_middle() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "1");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "5");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "12");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "99");
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

  auto split_partition_id = SHA1::compute("C");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "10"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(42);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 4);
  EXPECT_EQ(output[0].begin, encodePartitionKey(KEYSPACE_UINT64, "1"));
  EXPECT_EQ(output[0].end, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[1].end, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[2].end, encodePartitionKey(KEYSPACE_UINT64, "12"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[2].splitting, true);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[3].end, encodePartitionKey(KEYSPACE_UINT64, "99"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[3].splitting, false);

  MetadataFile input2(
      SHA1::compute("mytx2"),
      0,
      KEYSPACE_UINT64,
      output,
      MFILE_FINITE);

  FinalizeSplitOperation op2;
  op2.set_partition_id(split_partition_id.data(), split_partition_id.size());

  std::string opdata_buf2;
  op2.SerializeToString(&opdata_buf2);

  MetadataOperation openv2(
      "test",
      "test",
      METAOP_FINALIZE_SPLIT,
      SHA1::compute("mytx2"),
      SHA1::compute("mytxout2"),
      Buffer(opdata_buf2));

  EXPECT(input2.hasFinitePartitions() == true);

  Vector<MetadataFile::PartitionMapEntry> output2;
  rc = openv2.perform(input2, &output2);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output2.size(), 5);
  EXPECT_EQ(output2[0].begin, encodePartitionKey(KEYSPACE_UINT64, "1"));
  EXPECT_EQ(output2[0].end, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output2[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output2[0].splitting, false);
  EXPECT_EQ(output2[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output2[1].end, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output2[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output2[1].splitting, false);
  EXPECT_EQ(output2[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output2[2].end, encodePartitionKey(KEYSPACE_UINT64, "10"));
  EXPECT_EQ(output2[2].partition_id, SHA1::compute("X1"));
  EXPECT_EQ(output2[2].splitting, false);
  EXPECT_EQ(output2[2].servers.size(), 3);
  EXPECT_EQ(output2[2].servers[0].server_id, "l1");
  EXPECT_EQ(output2[2].servers[0].placement_id, 42);
  EXPECT_EQ(output2[2].servers[1].server_id, "l2");
  EXPECT_EQ(output2[2].servers[1].placement_id, 42);
  EXPECT_EQ(output2[2].servers[2].server_id, "l3");
  EXPECT_EQ(output2[2].servers[2].placement_id, 42);
  EXPECT_EQ(output2[3].begin, encodePartitionKey(KEYSPACE_UINT64, "10"));
  EXPECT_EQ(output2[3].end, encodePartitionKey(KEYSPACE_UINT64, "12"));
  EXPECT_EQ(output2[3].partition_id, SHA1::compute("X2"));
  EXPECT_EQ(output2[3].splitting, false);
  EXPECT_EQ(output2[3].servers.size(), 3);
  EXPECT_EQ(output2[3].servers[0].server_id, "h1");
  EXPECT_EQ(output2[3].servers[0].placement_id, 42);
  EXPECT_EQ(output2[3].servers[1].server_id, "h2");
  EXPECT_EQ(output2[3].servers[1].placement_id, 42);
  EXPECT_EQ(output2[3].servers[2].server_id, "h3");
  EXPECT_EQ(output2[3].servers[2].placement_id, 42);
  EXPECT_EQ(output2[4].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output2[4].end, encodePartitionKey(KEYSPACE_UINT64, "99"));
  EXPECT_EQ(output2[4].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output2[4].splitting, false);
  return true;
}

// UNIT-METAOP-SPLIT-012
static bool test_metadata_operation_split_finite_split_end() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "1");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "5");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "12");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "99");
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

  auto split_partition_id = SHA1::compute("D");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "50"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(42);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 4);
  EXPECT_EQ(output[0].begin, encodePartitionKey(KEYSPACE_UINT64, "1"));
  EXPECT_EQ(output[0].end, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[1].end, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[2].end, encodePartitionKey(KEYSPACE_UINT64, "12"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[3].end, encodePartitionKey(KEYSPACE_UINT64, "99"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[3].splitting, true);

  MetadataFile input2(
      SHA1::compute("mytx2"),
      0,
      KEYSPACE_UINT64,
      output,
      MFILE_FINITE);

  FinalizeSplitOperation op2;
  op2.set_partition_id(split_partition_id.data(), split_partition_id.size());

  std::string opdata_buf2;
  op2.SerializeToString(&opdata_buf2);

  MetadataOperation openv2(
      "test",
      "test",
      METAOP_FINALIZE_SPLIT,
      SHA1::compute("mytx2"),
      SHA1::compute("mytxout2"),
      Buffer(opdata_buf2));

  EXPECT(input2.hasFinitePartitions() == true);

  Vector<MetadataFile::PartitionMapEntry> output2;
  rc = openv2.perform(input2, &output2);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output2.size(), 5);
  EXPECT_EQ(output2[0].begin, encodePartitionKey(KEYSPACE_UINT64, "1"));
  EXPECT_EQ(output2[0].end, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output2[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output2[0].splitting, false);
  EXPECT_EQ(output2[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output2[1].end, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output2[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output2[1].splitting, false);
  EXPECT_EQ(output2[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output2[2].end, encodePartitionKey(KEYSPACE_UINT64, "12"));
  EXPECT_EQ(output2[2].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output2[2].splitting, false);
  EXPECT_EQ(output2[3].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output2[3].end, encodePartitionKey(KEYSPACE_UINT64, "50"));
  EXPECT_EQ(output2[3].partition_id, SHA1::compute("X1"));
  EXPECT_EQ(output2[3].splitting, false);
  EXPECT_EQ(output2[3].servers.size(), 3);
  EXPECT_EQ(output2[3].servers[0].server_id, "l1");
  EXPECT_EQ(output2[3].servers[0].placement_id, 42);
  EXPECT_EQ(output2[3].servers[1].server_id, "l2");
  EXPECT_EQ(output2[3].servers[1].placement_id, 42);
  EXPECT_EQ(output2[3].servers[2].server_id, "l3");
  EXPECT_EQ(output2[3].servers[2].placement_id, 42);
  EXPECT_EQ(output2[4].begin, encodePartitionKey(KEYSPACE_UINT64, "50"));
  EXPECT_EQ(output2[4].end, encodePartitionKey(KEYSPACE_UINT64, "99"));
  EXPECT_EQ(output2[4].partition_id, SHA1::compute("X2"));
  EXPECT_EQ(output2[4].splitting, false);
  EXPECT_EQ(output2[4].servers.size(), 3);
  EXPECT_EQ(output2[4].servers[0].server_id, "h1");
  EXPECT_EQ(output2[4].servers[0].placement_id, 42);
  EXPECT_EQ(output2[4].servers[1].server_id, "h2");
  EXPECT_EQ(output2[4].servers[1].placement_id, 42);
  EXPECT_EQ(output2[4].servers[2].server_id, "h3");
  EXPECT_EQ(output2[4].servers[2].placement_id, 42);
  return true;
}

// UNIT-METAOP-SPLIT-013
static bool test_metadata_operation_split_split_begin_immediate() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(SHA1::compute("mytx"), 0, KEYSPACE_UINT64, pmap, 0);

  auto split_partition_id = SHA1::compute("A");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_finalize_immediately(true);
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "2"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(23);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 5);
  EXPECT_EQ(output[0].begin, "");
  EXPECT_EQ(output[0].partition_id, SHA1::compute("X1"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[0].servers.size(), 3);
  EXPECT_EQ(output[0].servers[0].server_id, "l1");
  EXPECT_EQ(output[0].servers[0].placement_id, 23);
  EXPECT_EQ(output[0].servers[1].server_id, "l2");
  EXPECT_EQ(output[0].servers[1].placement_id, 23);
  EXPECT_EQ(output[0].servers[2].server_id, "l3");
  EXPECT_EQ(output[0].servers[2].placement_id, 23);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "2"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("X2"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[1].servers.size(), 3);
  EXPECT_EQ(output[1].servers[0].server_id, "h1");
  EXPECT_EQ(output[1].servers[0].placement_id, 23);
  EXPECT_EQ(output[1].servers[1].server_id, "h2");
  EXPECT_EQ(output[1].servers[1].placement_id, 23);
  EXPECT_EQ(output[1].servers[2].server_id, "h3");
  EXPECT_EQ(output[1].servers[2].placement_id, 23);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[3].splitting, false);
  EXPECT_EQ(output[4].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[4].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[4].splitting, false);
  return true;
}

// UNIT-METAOP-SPLIT-014
static bool test_metadata_operation_split_split_middle_immediate() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(SHA1::compute("mytx"), 0, KEYSPACE_UINT64, pmap, 0);

  auto split_partition_id = SHA1::compute("C");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_finalize_immediately(true);
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "10"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(42);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 5);
  EXPECT_EQ(output[0].begin, "");
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("X1"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[2].servers.size(), 3);
  EXPECT_EQ(output[2].servers[0].server_id, "l1");
  EXPECT_EQ(output[2].servers[0].placement_id, 42);
  EXPECT_EQ(output[2].servers[1].server_id, "l2");
  EXPECT_EQ(output[2].servers[1].placement_id, 42);
  EXPECT_EQ(output[2].servers[2].server_id, "l3");
  EXPECT_EQ(output[2].servers[2].placement_id, 42);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "10"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("X2"));
  EXPECT_EQ(output[3].splitting, false);
  EXPECT_EQ(output[3].servers.size(), 3);
  EXPECT_EQ(output[3].servers[0].server_id, "h1");
  EXPECT_EQ(output[3].servers[0].placement_id, 42);
  EXPECT_EQ(output[3].servers[1].server_id, "h2");
  EXPECT_EQ(output[3].servers[1].placement_id, 42);
  EXPECT_EQ(output[3].servers[2].server_id, "h3");
  EXPECT_EQ(output[3].servers[2].placement_id, 42);
  EXPECT_EQ(output[4].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[4].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[4].splitting, false);
  return true;
}

// UNIT-METAOP-SPLIT-015
static bool test_metadata_operation_split_split_end_immediate() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(SHA1::compute("mytx"), 0, KEYSPACE_UINT64, pmap, 0);

  auto split_partition_id = SHA1::compute("D");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_finalize_immediately(true);
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "23"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(17);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 5);
  EXPECT_EQ(output[0].begin, "");
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("X1"));
  EXPECT_EQ(output[3].splitting, false);
  EXPECT_EQ(output[3].servers.size(), 3);
  EXPECT_EQ(output[3].servers[0].server_id, "l1");
  EXPECT_EQ(output[3].servers[0].placement_id, 17);
  EXPECT_EQ(output[3].servers[1].server_id, "l2");
  EXPECT_EQ(output[3].servers[1].placement_id, 17);
  EXPECT_EQ(output[3].servers[2].server_id, "l3");
  EXPECT_EQ(output[3].servers[2].placement_id, 17);
  EXPECT_EQ(output[4].begin, encodePartitionKey(KEYSPACE_UINT64, "23"));
  EXPECT_EQ(output[4].partition_id, SHA1::compute("X2"));
  EXPECT_EQ(output[4].splitting, false);
  EXPECT_EQ(output[4].servers.size(), 3);
  EXPECT_EQ(output[4].servers[0].server_id, "h1");
  EXPECT_EQ(output[4].servers[0].placement_id, 17);
  EXPECT_EQ(output[4].servers[1].server_id, "h2");
  EXPECT_EQ(output[4].servers[1].placement_id, 17);
  EXPECT_EQ(output[4].servers[2].server_id, "h3");
  EXPECT_EQ(output[4].servers[2].placement_id, 17);
  return true;
}

// UNIT-METAOP-SPLIT-016
static bool test_metadata_operation_split_finite_split_begin_immediate() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "1");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "5");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "12");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "99");
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

  auto split_partition_id = SHA1::compute("A");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_finalize_immediately(true);
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "2"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(42);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 5);
  EXPECT_EQ(output[0].partition_id, SHA1::compute("X1"));
  EXPECT_EQ(output[0].begin, encodePartitionKey(KEYSPACE_UINT64, "1"));
  EXPECT_EQ(output[0].end, encodePartitionKey(KEYSPACE_UINT64, "2"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[0].servers.size(), 3);
  EXPECT_EQ(output[0].servers[0].server_id, "l1");
  EXPECT_EQ(output[0].servers[0].placement_id, 42);
  EXPECT_EQ(output[0].servers[1].server_id, "l2");
  EXPECT_EQ(output[0].servers[1].placement_id, 42);
  EXPECT_EQ(output[0].servers[2].server_id, "l3");
  EXPECT_EQ(output[0].servers[2].placement_id, 42);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "2"));
  EXPECT_EQ(output[1].end, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("X2"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[1].servers.size(), 3);
  EXPECT_EQ(output[1].servers[0].server_id, "h1");
  EXPECT_EQ(output[1].servers[0].placement_id, 42);
  EXPECT_EQ(output[1].servers[1].server_id, "h2");
  EXPECT_EQ(output[1].servers[1].placement_id, 42);
  EXPECT_EQ(output[1].servers[2].server_id, "h3");
  EXPECT_EQ(output[1].servers[2].placement_id, 42);
  EXPECT_EQ(output[2].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[2].end, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[3].end, encodePartitionKey(KEYSPACE_UINT64, "12"));
  EXPECT_EQ(output[3].splitting, false);
  EXPECT_EQ(output[4].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[4].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[4].end, encodePartitionKey(KEYSPACE_UINT64, "99"));
  EXPECT_EQ(output[4].splitting, false);
  return true;
}

// UNIT-METAOP-SPLIT-017
static bool test_metadata_operation_split_finite_split_middle_immediate() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "1");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "5");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "12");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "99");
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

  auto split_partition_id = SHA1::compute("C");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_finalize_immediately(true);
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "10"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(42);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 5);
  EXPECT_EQ(output[0].begin, encodePartitionKey(KEYSPACE_UINT64, "1"));
  EXPECT_EQ(output[0].end, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[1].end, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[2].end, encodePartitionKey(KEYSPACE_UINT64, "10"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("X1"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[2].servers.size(), 3);
  EXPECT_EQ(output[2].servers[0].server_id, "l1");
  EXPECT_EQ(output[2].servers[0].placement_id, 42);
  EXPECT_EQ(output[2].servers[1].server_id, "l2");
  EXPECT_EQ(output[2].servers[1].placement_id, 42);
  EXPECT_EQ(output[2].servers[2].server_id, "l3");
  EXPECT_EQ(output[2].servers[2].placement_id, 42);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "10"));
  EXPECT_EQ(output[3].end, encodePartitionKey(KEYSPACE_UINT64, "12"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("X2"));
  EXPECT_EQ(output[3].splitting, false);
  EXPECT_EQ(output[3].servers.size(), 3);
  EXPECT_EQ(output[3].servers[0].server_id, "h1");
  EXPECT_EQ(output[3].servers[0].placement_id, 42);
  EXPECT_EQ(output[3].servers[1].server_id, "h2");
  EXPECT_EQ(output[3].servers[1].placement_id, 42);
  EXPECT_EQ(output[3].servers[2].server_id, "h3");
  EXPECT_EQ(output[3].servers[2].placement_id, 42);
  EXPECT_EQ(output[4].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[4].end, encodePartitionKey(KEYSPACE_UINT64, "99"));
  EXPECT_EQ(output[4].partition_id, SHA1::compute("D"));
  EXPECT_EQ(output[4].splitting, false);
  return true;
}

// UNIT-METAOP-SPLIT-018
static bool test_metadata_operation_split_finite_split_end_immediate() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "1");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "5");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "12");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.end = encodePartitionKey(KEYSPACE_UINT64, "99");
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

  auto split_partition_id = SHA1::compute("D");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_finalize_immediately(true);
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "50"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(42);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(rc.isSuccess());
  EXPECT_EQ(output.size(), 5);
  EXPECT_EQ(output[0].begin, encodePartitionKey(KEYSPACE_UINT64, "1"));
  EXPECT_EQ(output[0].end, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[0].partition_id, SHA1::compute("A"));
  EXPECT_EQ(output[0].splitting, false);
  EXPECT_EQ(output[1].begin, encodePartitionKey(KEYSPACE_UINT64, "3"));
  EXPECT_EQ(output[1].end, encodePartitionKey(KEYSPACE_UINT64, "5"));
  EXPECT_EQ(output[1].partition_id, SHA1::compute("B"));
  EXPECT_EQ(output[1].splitting, false);
  EXPECT_EQ(output[2].begin, encodePartitionKey(KEYSPACE_UINT64, "8"));
  EXPECT_EQ(output[2].end, encodePartitionKey(KEYSPACE_UINT64, "12"));
  EXPECT_EQ(output[2].partition_id, SHA1::compute("C"));
  EXPECT_EQ(output[2].splitting, false);
  EXPECT_EQ(output[3].begin, encodePartitionKey(KEYSPACE_UINT64, "14"));
  EXPECT_EQ(output[3].end, encodePartitionKey(KEYSPACE_UINT64, "50"));
  EXPECT_EQ(output[3].partition_id, SHA1::compute("X1"));
  EXPECT_EQ(output[3].splitting, false);
  EXPECT_EQ(output[3].servers.size(), 3);
  EXPECT_EQ(output[3].servers[0].server_id, "l1");
  EXPECT_EQ(output[3].servers[0].placement_id, 42);
  EXPECT_EQ(output[3].servers[1].server_id, "l2");
  EXPECT_EQ(output[3].servers[1].placement_id, 42);
  EXPECT_EQ(output[3].servers[2].server_id, "l3");
  EXPECT_EQ(output[3].servers[2].placement_id, 42);
  EXPECT_EQ(output[4].begin, encodePartitionKey(KEYSPACE_UINT64, "50"));
  EXPECT_EQ(output[4].end, encodePartitionKey(KEYSPACE_UINT64, "99"));
  EXPECT_EQ(output[4].partition_id, SHA1::compute("X2"));
  EXPECT_EQ(output[4].splitting, false);
  EXPECT_EQ(output[4].servers.size(), 3);
  EXPECT_EQ(output[4].servers[0].server_id, "h1");
  EXPECT_EQ(output[4].servers[0].placement_id, 42);
  EXPECT_EQ(output[4].servers[1].server_id, "h2");
  EXPECT_EQ(output[4].servers[1].placement_id, 42);
  EXPECT_EQ(output[4].servers[2].server_id, "h3");
  EXPECT_EQ(output[4].servers[2].placement_id, 42);
  return true;
}

// UNIT-METAOP-SPLIT-019
static bool test_metadata_operation_split_user_defined_split() {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "1");
    e.partition_id = SHA1::compute("A");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "3");
    e.partition_id = SHA1::compute("B");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "8");
    e.partition_id = SHA1::compute("C");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = encodePartitionKey(KEYSPACE_UINT64, "14");
    e.partition_id = SHA1::compute("D");
    e.splitting = false;
    pmap.emplace_back(e);
  }

  MetadataFile input(
      SHA1::compute("mytx"),
      0,
      KEYSPACE_UINT64,
      pmap,
      MFILE_USERDEFINED);

  auto split_partition_id = SHA1::compute("A");
  auto split_partition_id_low = SHA1::compute("X1");
  auto split_partition_id_high = SHA1::compute("X2");

  SplitPartitionOperation op;
  op.set_partition_id(split_partition_id.data(), split_partition_id.size());
  op.set_split_point(encodePartitionKey(KEYSPACE_UINT64, "2"));
  op.set_split_partition_id_low(
      split_partition_id_low.data(),
      split_partition_id_low.size());
  op.set_split_partition_id_high(
      split_partition_id_high.data(),
      split_partition_id_high.size());
  op.set_placement_id(42);
  *op.add_split_servers_low() = "l1";
  *op.add_split_servers_low() = "l2";
  *op.add_split_servers_low() = "l3";
  *op.add_split_servers_high() = "h1";
  *op.add_split_servers_high() = "h2";
  *op.add_split_servers_high() = "h3";

  std::string opdata_buf;
  op.SerializeToString(&opdata_buf);

  MetadataOperation openv(
      "test",
      "test",
      METAOP_SPLIT_PARTITION,
      SHA1::compute("mytx"),
      SHA1::compute("mytxout"),
      Buffer(opdata_buf));

  Vector<MetadataFile::PartitionMapEntry> output;
  auto rc = openv.perform(input, &output);
  EXPECT(!rc.isSuccess());
  EXPECT(rc.message() == "can't split user defined partitions");
  return true;
}

void setup_unit_metadata_operation_split_tests(TestRepository* repo) {
  std::vector<TestCase> c;
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-001", metadata_operation_split, split_begin);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-002", metadata_operation_split, subsplit_begin_low);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-003", metadata_operation_split, subsplit_begin_high);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-004", metadata_operation_split, split_middle);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-005", metadata_operation_split, subsplit_middle_low);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-006", metadata_operation_split, subsplit_middle_high);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-007", metadata_operation_split, split_end);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-008", metadata_operation_split, subsplit_end_low);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-009", metadata_operation_split, subsplit_end_high);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-010", metadata_operation_split, finite_split_begin);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-011", metadata_operation_split, finite_split_middle);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-012", metadata_operation_split, finite_split_end);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-013", metadata_operation_split, split_begin_immediate);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-014", metadata_operation_split, split_middle_immediate);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-015", metadata_operation_split, split_end_immediate);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-016", metadata_operation_split, finite_split_begin_immediate);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-017", metadata_operation_split, finite_split_middle_immediate);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-018", metadata_operation_split, finite_split_end_immediate);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METAOP-SPLIT-019", metadata_operation_split, user_defined_split);
  repo->addTestBundle(c);
}

} // namespace unit
} // namespace test
} // namespace eventql

