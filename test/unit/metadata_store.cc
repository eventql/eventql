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
#include "eventql/util/fnv.h"
#include "eventql/db/metadata_store.h"

#include "../unit_test.h"
#include "../util/test_repository.h"

namespace eventql {
namespace test {
namespace unit {

// UNIT-METADATASTORE-001
static bool test_metadata_store_storeandload() {
  auto metadata_store_path = StringUtil::format(
      "/tmp/test_metadata_store-$0",
      Random::singleton()->hex64());

  FileUtil::mkdir_p(metadata_store_path);

  {
    MetadataStore metadata_store(metadata_store_path);
    Vector<MetadataFile::PartitionMapEntry> pmap;

    {
      MetadataFile::PartitionMapEntry e;
      e.begin = "a";
      e.splitting = false;

      {
        MetadataFile::PartitionPlacement p;
        p.server_id = "server1";
        p.placement_id = 0x42;
        e.servers.emplace_back(p);
      }

      {
        MetadataFile::PartitionPlacement p;
        p.server_id = "server2";
        p.placement_id = 0x23;
        e.servers.emplace_back(p);
      }

      pmap.emplace_back(e);
    }

    {
      MetadataFile::PartitionMapEntry e;
      e.begin = "b";
      e.splitting = true;
      e.split_point = "bx";
      e.split_partition_id_low = SHA1::compute("lowlow");
      e.split_partition_id_high = SHA1::compute("highhigh");

      {
        MetadataFile::PartitionPlacement p;
        p.server_id = "server3";
        p.placement_id = 0x42;
        e.servers_joining.emplace_back(p);
      }

      {
        MetadataFile::PartitionPlacement p;
        p.server_id = "server5";
        p.placement_id = 0x123;
        e.split_servers_low.emplace_back(p);
      }

      {
        MetadataFile::PartitionPlacement p;
        p.server_id = "server6";
        p.placement_id = 0x456;
        e.split_servers_high.emplace_back(p);
      }

      pmap.emplace_back(e);
    }


    MetadataFile file(SHA1::compute("mytx"), 7, KEYSPACE_STRING, pmap, 0);
    auto rc = metadata_store.storeMetadataFile(
        "ns",
        "mytbl",
        file);

    EXPECT(rc.isSuccess());
  }

  {
    MetadataStore metadata_store(metadata_store_path);
    RefPtr<MetadataFile> file;
    auto rc = metadata_store.getMetadataFile(
        "ns",
        "mytbl",
        SHA1::compute("mytx"),
        &file);

    EXPECT(rc.isSuccess());
    EXPECT(file->getTransactionID() == SHA1::compute("mytx"));
    EXPECT(file->getSequenceNumber() == 7);

    const auto& pmap = file->getPartitionMap();
    EXPECT_EQ(pmap.size(), 2);

    EXPECT_EQ(pmap[0].begin, "a");
    EXPECT_EQ(pmap[0].servers.size(), 2);
    EXPECT_EQ(pmap[0].servers[0].server_id, "server1");
    EXPECT_EQ(pmap[0].servers[0].placement_id, 0x42);
    EXPECT_EQ(pmap[0].servers[1].server_id, "server2");
    EXPECT_EQ(pmap[0].servers[1].placement_id, 0x23);
    EXPECT_EQ(pmap[0].servers_joining.size(), 0);
    EXPECT_EQ(pmap[0].splitting, false);
    EXPECT_EQ(pmap[0].split_point, "");
    EXPECT_EQ(pmap[0].split_servers_low.size(), 0);
    EXPECT_EQ(pmap[0].split_servers_high.size(), 0);

    EXPECT_EQ(pmap[1].begin, "b");
    EXPECT_EQ(pmap[1].servers.size(), 0);
    EXPECT_EQ(pmap[1].servers_joining.size(), 1);
    EXPECT_EQ(pmap[1].servers_joining[0].server_id, "server3");
    EXPECT_EQ(pmap[1].servers_joining[0].placement_id, 0x42);
    EXPECT_EQ(pmap[1].splitting, true);
    EXPECT_EQ(pmap[1].split_point, "bx");
    EXPECT_EQ(pmap[1].split_partition_id_low, SHA1::compute("lowlow"));
    EXPECT_EQ(pmap[1].split_partition_id_high, SHA1::compute("highhigh"));
    EXPECT_EQ(pmap[1].split_servers_low.size(), 1);
    EXPECT_EQ(pmap[1].split_servers_low[0].server_id, "server5");
    EXPECT_EQ(pmap[1].split_servers_low[0].placement_id, 0x123);
    EXPECT_EQ(pmap[1].split_servers_high.size(), 1);
    EXPECT_EQ(pmap[1].split_servers_high[0].server_id, "server6");
    EXPECT_EQ(pmap[1].split_servers_high[0].placement_id, 0x456);
  }

  return true;
}

// UNIT-METADATASTORE-002
static bool test_metadata_store_cache() {
  auto metadata_store_path = StringUtil::format(
      "/tmp/test_metadata_store-$0",
      Random::singleton()->hex64());

  FileUtil::mkdir_p(metadata_store_path);
  MetadataStore metadata_store(
      metadata_store_path,
      MetadataStore::kDefaultMaxBytes,
      12);

  for (size_t i = 0; i < 20; ++i) {
    auto txid = SHA1::compute(StringUtil::format("file$0", i % 20));
    MetadataFile file(txid, 1, KEYSPACE_STRING, {}, 0);
    auto rc = metadata_store.storeMetadataFile("ns", "mytbl", file);
    EXPECT(rc.isSuccess());
  }

  FNV<uint64_t> fnv;
  for (size_t i = 0; i < 10000; ++i) {
    auto idx = fnv.hash((const void*) &i, sizeof(i)) % 20;
    auto txid = SHA1::compute(StringUtil::format("file$0", idx));

    RefPtr<MetadataFile> file;
    auto rc = metadata_store.getMetadataFile(
        "ns",
        "mytbl",
        txid,
        &file);

    EXPECT(rc.isSuccess());
    EXPECT(file->getTransactionID() == txid);
  }

  return true;
}

void setup_unit_metadata_store_tests(TestRepository* repo) {
  std::vector<TestCase> c;
  SETUP_UNIT_TESTCASE(&c, "UNIT-METADATASTORE-001", metadata_store, storeandload);
  SETUP_UNIT_TESTCASE(&c, "UNIT-METADATASTORE-002", metadata_store, cache);
  repo->addTestBundle(c);
}

} // namespace unit
} // namespace test
} // namespace eventql

