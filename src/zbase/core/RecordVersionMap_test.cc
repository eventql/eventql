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
#include <stx/stdtypes.h>
#include <stx/test/unittest.h>
#include <zbase/core/RecordVersionMap.h>

using namespace stx;
using namespace zbase;

UNIT_TEST(RecordVersionMapTest);

using VersionMap = HashMap<SHA1Hash, uint64_t>;
using OrderedVersionMap = OrderedMap<SHA1Hash, uint64_t>;

TEST_CASE(RecordVersionMapTest, TestSimpleSet, [] () {
  auto filename = "/tmp/_zbase_recversionmap_test.idx";
  FileUtil::rm(filename);

  {
    OrderedVersionMap map;
    map[SHA1::compute("0x42424242")] = 3;
    map[SHA1::compute("0x23232323")] = 1;
    map[SHA1::compute("0x52525252")] = 2;
    RecordVersionMap::write(map, filename);
  }

  {
    VersionMap map;
    map[SHA1::compute("0x42424242")] = 0;
    map[SHA1::compute("0x23232323")] = 0;
    map[SHA1::compute("0x52525252")] = 0;
    map[SHA1::compute("0x17171717")] = 0;
    RecordVersionMap::lookup(&map, filename);
    EXPECT_EQ(map.size(), 4);
    EXPECT_EQ(map[SHA1::compute("0x42424242")], 3);
    EXPECT_EQ(map[SHA1::compute("0x23232323")], 1);
    EXPECT_EQ(map[SHA1::compute("0x52525252")], 2);
    EXPECT_EQ(map[SHA1::compute("0x17171717")], 0);
  }

  {
    VersionMap map;
    map[SHA1::compute("0x52525252")] = 0;
    RecordVersionMap::lookup(&map, filename);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[SHA1::compute("0x52525252")], 2);
  }

  {
    VersionMap map;
    map[SHA1::compute("0x52525252")] = 3;
    RecordVersionMap::lookup(&map, filename);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[SHA1::compute("0x52525252")], 3);
  }

  {
    VersionMap map;
    RecordVersionMap::load(&map, filename);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map[SHA1::compute("0x42424242")], 3);
    EXPECT_EQ(map[SHA1::compute("0x23232323")], 1);
    EXPECT_EQ(map[SHA1::compute("0x52525252")], 2);
  }

});


