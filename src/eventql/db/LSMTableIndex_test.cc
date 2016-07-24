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
#include <eventql/util/stdtypes.h>
#include <eventql/util/test/unittest.h>
#include <eventql/db/LSMTableIndex.h>

#include "eventql/eventql.h"
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

