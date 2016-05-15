/**
 * Copyright (c) 2015 - zScale Technology GmbH <legal@zscale.io>
 *   All Rights Reserved.
 *
 * Authors:
 *   Paul Asmuth <paul@zscale.io>
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <eventql/util/stdtypes.h>
#include <eventql/util/test/unittest.h>
#include <eventql/core/LSMTableIndex.h>

using namespace util;
using namespace eventql;

UNIT_TEST(LSMTableIndexTest);

using VersionMap = HashMap<SHA1Hash, uint64_t>;
using OrderedVersionMap = OrderedMap<SHA1Hash, uint64_t>;

TEST_CASE(LSMTableIndexTest, TestLookup, [] () {
  auto filename = "/tmp/_eventql_recversionmap_test.idx";
  FileUtil::rm(filename);

  {
    OrderedVersionMap map;
    map[SHA1::compute("0x42424242")] = 3;
    map[SHA1::compute("0x23232323")] = 1;
    map[SHA1::compute("0x52525252")] = 2;
    LSMTableIndex::write(map, filename);
  }

  {
    VersionMap map;
    map[SHA1::compute("0x42424242")] = 0;
    map[SHA1::compute("0x23232323")] = 0;
    map[SHA1::compute("0x52525252")] = 0;
    map[SHA1::compute("0x17171717")] = 0;
    LSMTableIndex idx(filename);
    idx.lookup(&map);
    EXPECT_EQ(map.size(), 4);
    EXPECT_EQ(map[SHA1::compute("0x42424242")], 3);
    EXPECT_EQ(map[SHA1::compute("0x23232323")], 1);
    EXPECT_EQ(map[SHA1::compute("0x52525252")], 2);
    EXPECT_EQ(map[SHA1::compute("0x17171717")], 0);
  }

  {
    VersionMap map;
    map[SHA1::compute("0x52525252")] = 0;
    LSMTableIndex idx(filename);
    idx.lookup(&map);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[SHA1::compute("0x52525252")], 2);
  }

  {
    VersionMap map;
    map[SHA1::compute("0x52525252")] = 3;
    LSMTableIndex idx(filename);
    idx.lookup(&map);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[SHA1::compute("0x52525252")], 3);
  }

  {
    VersionMap map;
    LSMTableIndex idx(filename);
    idx.list(&map);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map[SHA1::compute("0x42424242")], 3);
    EXPECT_EQ(map[SHA1::compute("0x23232323")], 1);
    EXPECT_EQ(map[SHA1::compute("0x52525252")], 2);
  }
});

TEST_CASE(LSMTableIndexTest, TestEmptyMap, [] () {
  auto filename = "/tmp/_eventql_recversionmap_test.idx";
  FileUtil::rm(filename);

  {
    OrderedVersionMap map;
    LSMTableIndex::write(map, filename);
  }

  {
    VersionMap map;
    map[SHA1::compute("0x42424242")] = 0;
    map[SHA1::compute("0x23232323")] = 0;
    map[SHA1::compute("0x52525252")] = 0;
    map[SHA1::compute("0x17171717")] = 0;
    LSMTableIndex idx(filename);
    idx.lookup(&map);
    EXPECT_EQ(map.size(), 4);
    EXPECT_EQ(map[SHA1::compute("0x42424242")], 0);
    EXPECT_EQ(map[SHA1::compute("0x23232323")], 0);
    EXPECT_EQ(map[SHA1::compute("0x52525252")], 0);
    EXPECT_EQ(map[SHA1::compute("0x17171717")], 0);
  }

  {
    VersionMap map;
    LSMTableIndex idx(filename);
    idx.list(&map);
    EXPECT_EQ(map.size(), 0);
  }
});

TEST_CASE(LSMTableIndexTest, TestMapWithOneSlot, [] () {
  auto filename = "/tmp/_eventql_recversionmap_test.idx";
  FileUtil::rm(filename);

  {
    OrderedVersionMap map;
    map[SHA1::compute("0x23232323")] = 1;
    LSMTableIndex::write(map, filename);
  }

  {
    VersionMap map;
    map[SHA1::compute("0x42424242")] = 0;
    map[SHA1::compute("0x23232323")] = 0;
    map[SHA1::compute("0x52525252")] = 0;
    map[SHA1::compute("0x17171717")] = 0;
    LSMTableIndex idx(filename);
    idx.lookup(&map);
    EXPECT_EQ(map.size(), 4);
    EXPECT_EQ(map[SHA1::compute("0x42424242")], 0);
    EXPECT_EQ(map[SHA1::compute("0x23232323")], 1);
    EXPECT_EQ(map[SHA1::compute("0x52525252")], 0);
    EXPECT_EQ(map[SHA1::compute("0x17171717")], 0);
  }

  {
    VersionMap map;
    LSMTableIndex idx(filename);
    idx.list(&map);
    EXPECT_EQ(map.size(), 1);
  }
});

