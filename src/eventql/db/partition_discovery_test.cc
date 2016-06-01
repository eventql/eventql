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

  MetadataFile file(SHA1::compute("mytx"), 0, KEYSPACE_STRING, pmap);

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
    EXPECT(res.replication_targets().size() == 2);
    EXPECT(res.replication_targets().Get(0).server_id() == "s4");
    EXPECT(res.replication_targets().Get(0).placement_id() == 13);
    EXPECT(res.replication_targets().Get(1).server_id() == "s1");
    EXPECT(res.replication_targets().Get(1).placement_id() == 12);
  }
});

