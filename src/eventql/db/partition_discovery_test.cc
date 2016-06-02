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
#include "eventql/eventql.h"
#include "eventql/util/stdtypes.h"
#include "eventql/util/random.h"
#include "eventql/util/test/unittest.h"
#include "eventql/db/metadata_store.h"
#include "eventql/db/metadata_operations.pb.h"
#include "eventql/db/partition_discovery.h"

using namespace eventql;

UNIT_TEST(PartitionDiscoveryTest);

// serving
// loading
// splitted exact 2 way
// splitted 2^n ways
// splitted three way weird
// not yet created // out of valid range

static MetadataFile::PartitionPlacement mkPlacement(
    const String& server_id,
    uint64_t placement_id) {
  MetadataFile::PartitionPlacement p;
  p.server_id = server_id;
  p.placement_id = placement_id;
  return p;
}

TEST_CASE(PartitionDiscoveryTest, TestServingPartition, [] () {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.splitting = false;
    e.partition_id = SHA1::compute("1");
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "b";
    e.splitting = false;
    e.partition_id = SHA1::compute("2");
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "d";
    e.splitting = false;
    e.partition_id = SHA1::compute("3");
    e.servers.emplace_back(mkPlacement("s6", 13));
    e.servers.emplace_back(mkPlacement("s3", 11));
    e.servers.emplace_back(mkPlacement("s2", 12));
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "e";
    e.splitting = false;
    e.partition_id = SHA1::compute("4");
    e.servers.emplace_back(mkPlacement("s4", 13));
    e.servers.emplace_back(mkPlacement("s2", 11));
    e.servers.emplace_back(mkPlacement("s1", 12));
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "g";
    e.splitting = false;
    e.partition_id = SHA1::compute("5");
    e.servers.emplace_back(mkPlacement("s1", 10));
    e.servers.emplace_back(mkPlacement("s2", 11));
    e.servers.emplace_back(mkPlacement("s3", 12));
    pmap.emplace_back(e);
  }

  MetadataFile file(SHA1::compute("mytx"), 7, KEYSPACE_STRING, pmap);

  {
    auto pid = SHA1::compute("4");
    PartitionDiscoveryRequest req;
    req.set_db_namespace("test");
    req.set_table_id("test");
    req.set_min_txnseq(1);
    req.set_partition_id(pid.data(), pid.size());
    req.set_keyrange_begin("e");
    req.set_keyrange_end("g");
    req.set_requester_id("sX");
    PartitionDiscoveryResponse res;
    auto rc = PartitionDiscovery::discoverPartition(&file, req, &res);
    EXPECT(rc.isSuccess());
    EXPECT(res.code() == PDISCOVERY_UNLOAD);
    EXPECT(SHA1Hash(res.txnid().data(), res.txnid().size()) == SHA1::compute("mytx"));
    EXPECT(res.txnseq() ==  7);
    EXPECT(res.replication_targets().size() == 3);
    EXPECT(res.replication_targets().Get(0).server_id() == "s4");
    EXPECT(res.replication_targets().Get(0).placement_id() == 13);
    EXPECT(res.replication_targets().Get(1).server_id() == "s2");
    EXPECT(res.replication_targets().Get(1).placement_id() == 11);
    EXPECT(res.replication_targets().Get(2).server_id() == "s1");
    EXPECT(res.replication_targets().Get(2).placement_id() == 12);
  }

  {
    auto pid = SHA1::compute("4");
    PartitionDiscoveryRequest req;
    req.set_db_namespace("test");
    req.set_table_id("test");
    req.set_min_txnseq(1);
    req.set_partition_id(pid.data(), pid.size());
    req.set_keyrange_begin("e");
    req.set_keyrange_end("g");
    req.set_requester_id("s2");
    PartitionDiscoveryResponse res;
    auto rc = PartitionDiscovery::discoverPartition(&file, req, &res);
    EXPECT(rc.isSuccess());
    EXPECT(res.code() == PDISCOVERY_SERVE);
    EXPECT(SHA1Hash(res.txnid().data(), res.txnid().size()) == SHA1::compute("mytx"));
    EXPECT(res.txnseq() == 7);
    EXPECT(res.replication_targets().size() == 2);
    EXPECT(res.replication_targets().Get(0).server_id() == "s4");
    EXPECT(res.replication_targets().Get(0).placement_id() == 13);
    EXPECT(res.replication_targets().Get(1).server_id() == "s1");
    EXPECT(res.replication_targets().Get(1).placement_id() == 12);
    EXPECT(res.keyrange_begin() == "e");
    EXPECT(res.keyrange_end() == "g");
  }
});

TEST_CASE(PartitionDiscoveryTest, TestSplittingPartition, [] () {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "a";
    e.splitting = false;
    e.partition_id = SHA1::compute("3");
    e.servers.emplace_back(mkPlacement("s6", 13));
    e.servers.emplace_back(mkPlacement("s3", 11));
    e.servers.emplace_back(mkPlacement("s2", 12));
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "e";
    e.partition_id = SHA1::compute("4");
    e.servers.emplace_back(mkPlacement("s4", 13));
    e.servers.emplace_back(mkPlacement("s3", 11));
    e.servers.emplace_back(mkPlacement("s1", 12));
    e.splitting = true;
    e.split_point = "p";
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
    e.begin = "x";
    e.splitting = false;
    e.partition_id = SHA1::compute("5");
    e.servers.emplace_back(mkPlacement("s1", 10));
    e.servers.emplace_back(mkPlacement("s2", 11));
    e.servers.emplace_back(mkPlacement("s3", 12));
    pmap.emplace_back(e);
  }

  MetadataFile file(SHA1::compute("mytx"), 7, KEYSPACE_STRING, pmap);

  {
    auto pid = SHA1::compute("4");
    PartitionDiscoveryRequest req;
    req.set_db_namespace("test");
    req.set_table_id("test");
    req.set_min_txnseq(1);
    req.set_partition_id(pid.data(), pid.size());
    req.set_keyrange_begin("e");
    req.set_keyrange_end("x");
    req.set_requester_id("s3");
    PartitionDiscoveryResponse res;
    auto rc = PartitionDiscovery::discoverPartition(&file, req, &res);
    EXPECT(rc.isSuccess());
    EXPECT(res.code() == PDISCOVERY_SERVE);
    EXPECT(SHA1Hash(res.txnid().data(), res.txnid().size()) == SHA1::compute("mytx"));
    EXPECT(res.txnseq() ==  7);
    EXPECT(res.replication_targets().size() == 8);
    EXPECT(res.replication_targets().Get(0).server_id() == "s4");
    EXPECT(res.replication_targets().Get(0).placement_id() == 13);
    EXPECT(res.replication_targets().Get(0).keyrange_begin() == "e");
    EXPECT(res.replication_targets().Get(0).keyrange_end() == "x");
    EXPECT(res.replication_targets().Get(1).server_id() == "s1");
    EXPECT(res.replication_targets().Get(1).placement_id() == 12);
    EXPECT(res.replication_targets().Get(1).keyrange_begin() == "e");
    EXPECT(res.replication_targets().Get(1).keyrange_end() == "x");
    EXPECT(res.replication_targets().Get(2).server_id() == "s8");
    EXPECT(res.replication_targets().Get(2).placement_id() == 43);
    EXPECT(res.replication_targets().Get(2).keyrange_begin() == "e");
    EXPECT(res.replication_targets().Get(2).keyrange_end() == "p");
    EXPECT(res.replication_targets().Get(3).server_id() == "s9");
    EXPECT(res.replication_targets().Get(3).placement_id() == 41);
    EXPECT(res.replication_targets().Get(3).keyrange_begin() == "e");
    EXPECT(res.replication_targets().Get(3).keyrange_end() == "p");
    EXPECT(res.replication_targets().Get(4).server_id() == "s6");
    EXPECT(res.replication_targets().Get(4).placement_id() == 42);
    EXPECT(res.replication_targets().Get(4).keyrange_begin() == "e");
    EXPECT(res.replication_targets().Get(4).keyrange_end() == "p");
    EXPECT(res.replication_targets().Get(5).server_id() == "s5");
    EXPECT(res.replication_targets().Get(5).placement_id() == 51);
    EXPECT(res.replication_targets().Get(5).keyrange_begin() == "p");
    EXPECT(res.replication_targets().Get(5).keyrange_end() == "x");
    EXPECT(res.replication_targets().Get(6).server_id() == "s7");
    EXPECT(res.replication_targets().Get(6).placement_id() == 52);
    EXPECT(res.replication_targets().Get(6).keyrange_begin() == "p");
    EXPECT(res.replication_targets().Get(6).keyrange_end() == "x");
    EXPECT(res.replication_targets().Get(7).server_id() == "s3");
    EXPECT(res.replication_targets().Get(7).placement_id() == 53);
    EXPECT(res.replication_targets().Get(7).keyrange_begin() == "p");
    EXPECT(res.replication_targets().Get(7).keyrange_end() == "x");
  }

  {
    auto pid = SHA1::compute("x");
    PartitionDiscoveryRequest req;
    req.set_db_namespace("test");
    req.set_table_id("test");
    req.set_min_txnseq(1);
    req.set_partition_id(pid.data(), pid.size());
    req.set_keyrange_begin("e");
    req.set_keyrange_end("z");
    req.set_requester_id("s3");
    PartitionDiscoveryResponse res;
    auto rc = PartitionDiscovery::discoverPartition(&file, req, &res);
    EXPECT(rc.isSuccess());
    EXPECT(res.code() == PDISCOVERY_UNLOAD);
    EXPECT(SHA1Hash(res.txnid().data(), res.txnid().size()) == SHA1::compute("mytx"));
    EXPECT(res.txnseq() ==  7);
    EXPECT_EQ(res.replication_targets().size(), 12);
    EXPECT(res.replication_targets().Get(0).server_id() == "s4");
    EXPECT(res.replication_targets().Get(0).placement_id() == 13);
    EXPECT(res.replication_targets().Get(0).keyrange_begin() == "e");
    EXPECT(res.replication_targets().Get(0).keyrange_end() == "x");
    EXPECT(res.replication_targets().Get(1).server_id() == "s3");
    EXPECT(res.replication_targets().Get(1).placement_id() == 11);
    EXPECT(res.replication_targets().Get(1).keyrange_begin() == "e");
    EXPECT(res.replication_targets().Get(1).keyrange_end() == "x");
    EXPECT(res.replication_targets().Get(2).server_id() == "s1");
    EXPECT(res.replication_targets().Get(2).placement_id() == 12);
    EXPECT(res.replication_targets().Get(2).keyrange_begin() == "e");
    EXPECT(res.replication_targets().Get(2).keyrange_end() == "x");
    EXPECT(res.replication_targets().Get(3).server_id() == "s8");
    EXPECT(res.replication_targets().Get(3).placement_id() == 43);
    EXPECT(res.replication_targets().Get(3).keyrange_begin() == "e");
    EXPECT(res.replication_targets().Get(3).keyrange_end() == "p");
    EXPECT(res.replication_targets().Get(4).server_id() == "s9");
    EXPECT(res.replication_targets().Get(4).placement_id() == 41);
    EXPECT(res.replication_targets().Get(4).keyrange_begin() == "e");
    EXPECT(res.replication_targets().Get(4).keyrange_end() == "p");
    EXPECT(res.replication_targets().Get(5).server_id() == "s6");
    EXPECT(res.replication_targets().Get(5).placement_id() == 42);
    EXPECT(res.replication_targets().Get(5).keyrange_begin() == "e");
    EXPECT(res.replication_targets().Get(5).keyrange_end() == "p");
    EXPECT(res.replication_targets().Get(6).server_id() == "s5");
    EXPECT(res.replication_targets().Get(6).placement_id() == 51);
    EXPECT(res.replication_targets().Get(6).keyrange_begin() == "p");
    EXPECT(res.replication_targets().Get(6).keyrange_end() == "x");
    EXPECT(res.replication_targets().Get(7).server_id() == "s7");
    EXPECT(res.replication_targets().Get(7).placement_id() == 52);
    EXPECT(res.replication_targets().Get(7).keyrange_begin() == "p");
    EXPECT(res.replication_targets().Get(7).keyrange_end() == "x");
    EXPECT(res.replication_targets().Get(8).server_id() == "s3");
    EXPECT(res.replication_targets().Get(8).placement_id() == 53);
    EXPECT(res.replication_targets().Get(8).keyrange_begin() == "p");
    EXPECT(res.replication_targets().Get(8).keyrange_end() == "x");
    EXPECT(res.replication_targets().Get(9).server_id() == "s1");
    EXPECT(res.replication_targets().Get(9).placement_id() == 10);
    EXPECT(res.replication_targets().Get(9).keyrange_begin() == "x");
    EXPECT(res.replication_targets().Get(9).keyrange_end() == "");
    EXPECT(res.replication_targets().Get(10).server_id() == "s2");
    EXPECT(res.replication_targets().Get(10).placement_id() == 11);
    EXPECT(res.replication_targets().Get(10).keyrange_begin() == "x");
    EXPECT(res.replication_targets().Get(10).keyrange_end() == "");
    EXPECT(res.replication_targets().Get(11).server_id() == "s3");
    EXPECT(res.replication_targets().Get(11).placement_id() == 12);
    EXPECT(res.replication_targets().Get(11).keyrange_begin() == "x");
    EXPECT(res.replication_targets().Get(11).keyrange_end() == "");
  }

  {
    auto pid = SHA1::compute("lowlow");
    PartitionDiscoveryRequest req;
    req.set_db_namespace("test");
    req.set_table_id("test");
    req.set_min_txnseq(1);
    req.set_partition_id(pid.data(), pid.size());
    req.set_keyrange_begin("e");
    req.set_keyrange_end("p");
    req.set_requester_id("s9");
    PartitionDiscoveryResponse res;
    auto rc = PartitionDiscovery::discoverPartition(&file, req, &res);
    EXPECT(rc.isSuccess());
    EXPECT(res.code() == PDISCOVERY_LOAD);
    EXPECT(SHA1Hash(res.txnid().data(), res.txnid().size()) == SHA1::compute("mytx"));
    EXPECT(res.txnseq() ==  7);
    EXPECT(res.replication_targets().size() == 2);
    EXPECT(res.replication_targets().Get(0).server_id() == "s8");
    EXPECT(res.replication_targets().Get(0).placement_id() == 43);
    EXPECT_EQ(res.replication_targets().Get(0).keyrange_begin(), "e");
    EXPECT_EQ(res.replication_targets().Get(0).keyrange_end(), "p");
    EXPECT(res.replication_targets().Get(1).server_id() == "s6");
    EXPECT(res.replication_targets().Get(1).placement_id() == 42);
    EXPECT(res.replication_targets().Get(1).keyrange_begin() == "e");
    EXPECT(res.replication_targets().Get(1).keyrange_end() == "p");
  }

  {
    auto pid = SHA1::compute("lowlow");
    PartitionDiscoveryRequest req;
    req.set_db_namespace("test");
    req.set_table_id("test");
    req.set_min_txnseq(1);
    req.set_partition_id(pid.data(), pid.size());
    req.set_lookup_by_id(true);
    req.set_requester_id("s9");
    PartitionDiscoveryResponse res;
    auto rc = PartitionDiscovery::discoverPartition(&file, req, &res);
    EXPECT(rc.isSuccess());
    EXPECT(res.code() == PDISCOVERY_LOAD);
    EXPECT(SHA1Hash(res.txnid().data(), res.txnid().size()) == SHA1::compute("mytx"));
    EXPECT(res.txnseq() ==  7);
    EXPECT(res.keyrange_begin() == "e");
    EXPECT(res.keyrange_end() == "p");
    EXPECT(res.replication_targets().size() == 2);
    EXPECT(res.replication_targets().Get(0).server_id() == "s8");
    EXPECT(res.replication_targets().Get(0).placement_id() == 43);
    EXPECT(res.replication_targets().Get(0).keyrange_begin() == "e");
    EXPECT(res.replication_targets().Get(0).keyrange_end() == "p");
    EXPECT(res.replication_targets().Get(1).server_id() == "s6");
    EXPECT(res.replication_targets().Get(1).placement_id() == 42);
    EXPECT(res.replication_targets().Get(1).keyrange_begin() == "e");
    EXPECT(res.replication_targets().Get(1).keyrange_end() == "p");
  }

  {
    auto pid = SHA1::compute("highhigh");
    PartitionDiscoveryRequest req;
    req.set_db_namespace("test");
    req.set_table_id("test");
    req.set_min_txnseq(1);
    req.set_partition_id(pid.data(), pid.size());
    req.set_keyrange_begin("e");
    req.set_keyrange_end("p");
    req.set_requester_id("s5");
    PartitionDiscoveryResponse res;
    auto rc = PartitionDiscovery::discoverPartition(&file, req, &res);
    EXPECT(rc.isSuccess());
    EXPECT(res.code() == PDISCOVERY_LOAD);
    EXPECT(SHA1Hash(res.txnid().data(), res.txnid().size()) == SHA1::compute("mytx"));
    EXPECT(res.txnseq() ==  7);
    EXPECT(res.replication_targets().size() == 2);
    EXPECT(res.replication_targets().Get(0).server_id() == "s7");
    EXPECT(res.replication_targets().Get(0).placement_id() == 52);
    EXPECT(res.replication_targets().Get(0).keyrange_begin() == "p");
    EXPECT(res.replication_targets().Get(0).keyrange_end() == "x");
    EXPECT(res.replication_targets().Get(1).server_id() == "s3");
    EXPECT(res.replication_targets().Get(1).placement_id() == 53);
    EXPECT(res.replication_targets().Get(1).keyrange_begin() == "p");
    EXPECT(res.replication_targets().Get(1).keyrange_end() == "x");
  }

  {
    auto pid = SHA1::compute("highhigh");
    PartitionDiscoveryRequest req;
    req.set_db_namespace("test");
    req.set_table_id("test");
    req.set_min_txnseq(1);
    req.set_partition_id(pid.data(), pid.size());
    req.set_lookup_by_id(true);
    req.set_requester_id("s5");
    PartitionDiscoveryResponse res;
    auto rc = PartitionDiscovery::discoverPartition(&file, req, &res);
    EXPECT(rc.isSuccess());
    EXPECT(res.code() == PDISCOVERY_LOAD);
    EXPECT(SHA1Hash(res.txnid().data(), res.txnid().size()) == SHA1::compute("mytx"));
    EXPECT(res.txnseq() ==  7);
    EXPECT(res.keyrange_begin() == "p");
    EXPECT(res.keyrange_end() == "x");
    EXPECT(res.replication_targets().size() == 2);
    EXPECT(res.replication_targets().Get(0).server_id() == "s7");
    EXPECT(res.replication_targets().Get(0).placement_id() == 52);
    EXPECT(res.replication_targets().Get(0).keyrange_begin() == "p");
    EXPECT(res.replication_targets().Get(0).keyrange_end() == "x");
    EXPECT(res.replication_targets().Get(1).server_id() == "s3");
    EXPECT(res.replication_targets().Get(1).placement_id() == 53);
    EXPECT(res.replication_targets().Get(1).keyrange_begin() == "p");
    EXPECT(res.replication_targets().Get(1).keyrange_end() == "x");
  }
});

TEST_CASE(PartitionDiscoveryTest, TestFindByID, [] () {
  Vector<MetadataFile::PartitionMapEntry> pmap;

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "";
    e.splitting = false;
    e.partition_id = SHA1::compute("1");
    e.servers.emplace_back(mkPlacement("s6", 13));
    e.servers.emplace_back(mkPlacement("s3", 11));
    e.servers.emplace_back(mkPlacement("s2", 12));
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "b";
    e.splitting = false;
    e.partition_id = SHA1::compute("2");
    e.servers_joining.emplace_back(mkPlacement("s6", 13));
    e.servers.emplace_back(mkPlacement("s3", 11));
    e.servers.emplace_back(mkPlacement("s2", 12));
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "d";
    e.splitting = false;
    e.partition_id = SHA1::compute("3");
    e.servers.emplace_back(mkPlacement("s6", 13));
    e.servers.emplace_back(mkPlacement("s3", 11));
    e.servers.emplace_back(mkPlacement("s2", 12));
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "e";
    e.splitting = false;
    e.partition_id = SHA1::compute("4");
    e.servers.emplace_back(mkPlacement("s4", 13));
    e.servers.emplace_back(mkPlacement("s2", 11));
    e.servers.emplace_back(mkPlacement("s1", 12));
    pmap.emplace_back(e);
  }

  {
    MetadataFile::PartitionMapEntry e;
    e.begin = "g";
    e.splitting = false;
    e.partition_id = SHA1::compute("5");
    e.servers.emplace_back(mkPlacement("s1", 10));
    e.servers.emplace_back(mkPlacement("s2", 11));
    e.servers.emplace_back(mkPlacement("s3", 12));
    pmap.emplace_back(e);
  }

  MetadataFile file(SHA1::compute("mytx"), 7, KEYSPACE_STRING, pmap);

  {
    auto pid = SHA1::compute("x");
    PartitionDiscoveryRequest req;
    req.set_db_namespace("test");
    req.set_table_id("test");
    req.set_min_txnseq(0);
    req.set_partition_id(pid.data(), pid.size());
    req.set_requester_id("s3");
    req.set_lookup_by_id(true);
    PartitionDiscoveryResponse res;
    auto rc = PartitionDiscovery::discoverPartition(&file, req, &res);
    EXPECT(rc.isSuccess());
    EXPECT(res.code() == PDISCOVERY_UNKNOWN);
  }

  {
    auto pid = SHA1::compute("1");
    PartitionDiscoveryRequest req;
    req.set_db_namespace("test");
    req.set_table_id("test");
    req.set_min_txnseq(0);
    req.set_partition_id(pid.data(), pid.size());
    req.set_requester_id("sX");
    req.set_lookup_by_id(true);
    PartitionDiscoveryResponse res;
    auto rc = PartitionDiscovery::discoverPartition(&file, req, &res);
    EXPECT(rc.isSuccess());
    EXPECT(res.code() == PDISCOVERY_UNKNOWN);
  }

  {
    auto pid = SHA1::compute("1");
    PartitionDiscoveryRequest req;
    req.set_db_namespace("test");
    req.set_table_id("test");
    req.set_min_txnseq(0);
    req.set_partition_id(pid.data(), pid.size());
    req.set_requester_id("s3");
    req.set_lookup_by_id(true);
    PartitionDiscoveryResponse res;
    auto rc = PartitionDiscovery::discoverPartition(&file, req, &res);
    EXPECT(rc.isSuccess());
    EXPECT(res.code() == PDISCOVERY_SERVE);
    EXPECT(SHA1Hash(res.txnid().data(), res.txnid().size()) == SHA1::compute("mytx"));
    EXPECT(res.txnseq() ==  7);
    EXPECT(res.replication_targets().size() == 2);
    EXPECT(res.replication_targets().Get(0).server_id() == "s6");
    EXPECT(res.replication_targets().Get(0).placement_id() == 13);
    EXPECT(res.replication_targets().Get(1).server_id() == "s2");
    EXPECT(res.replication_targets().Get(1).placement_id() == 12);
    EXPECT(res.keyrange_begin() == "");
    EXPECT(res.keyrange_end() == "b");
  }

  {
    auto pid = SHA1::compute("2");
    PartitionDiscoveryRequest req;
    req.set_db_namespace("test");
    req.set_table_id("test");
    req.set_min_txnseq(0);
    req.set_partition_id(pid.data(), pid.size());
    req.set_requester_id("s6");
    req.set_lookup_by_id(true);
    PartitionDiscoveryResponse res;
    auto rc = PartitionDiscovery::discoverPartition(&file, req, &res);
    EXPECT(rc.isSuccess());
    EXPECT(res.code() == PDISCOVERY_LOAD);
    EXPECT(SHA1Hash(res.txnid().data(), res.txnid().size()) == SHA1::compute("mytx"));
    EXPECT(res.txnseq() ==  7);
    EXPECT(res.replication_targets().size() == 2);
    EXPECT(res.replication_targets().Get(0).server_id() == "s3");
    EXPECT(res.replication_targets().Get(0).placement_id() == 11);
    EXPECT(res.replication_targets().Get(1).server_id() == "s2");
    EXPECT(res.replication_targets().Get(1).placement_id() == 12);
    EXPECT(res.keyrange_begin() == "b");
    EXPECT(res.keyrange_end() == "d");
  }

  {
    auto pid = SHA1::compute("5");
    PartitionDiscoveryRequest req;
    req.set_db_namespace("test");
    req.set_table_id("test");
    req.set_min_txnseq(0);
    req.set_partition_id(pid.data(), pid.size());
    req.set_requester_id("s3");
    req.set_lookup_by_id(true);
    PartitionDiscoveryResponse res;
    auto rc = PartitionDiscovery::discoverPartition(&file, req, &res);
    EXPECT(rc.isSuccess());
    EXPECT(res.code() == PDISCOVERY_SERVE);
    EXPECT(SHA1Hash(res.txnid().data(), res.txnid().size()) == SHA1::compute("mytx"));
    EXPECT(res.txnseq() ==  7);
    EXPECT(res.replication_targets().size() == 2);
    EXPECT(res.replication_targets().Get(0).server_id() == "s1");
    EXPECT(res.replication_targets().Get(0).placement_id() == 10);
    EXPECT(res.replication_targets().Get(1).server_id() == "s2");
    EXPECT(res.replication_targets().Get(1).placement_id() == 11);
    EXPECT(res.keyrange_begin() == "g");
    EXPECT(res.keyrange_end() == "");
  }
});

