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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "eventql/util/test/unittest.h"
#include "eventql/core/Partition.h"
#include "eventql/core/TimeWindowPartitioner.h"

using namespace stx;
using namespace eventql;

// FIXPAUL rename to TimeWindowPartitionerTest
UNIT_TEST(TimeWindowPartitionerTest);

TEST_CASE(TimeWindowPartitionerTest, TestStreamKeyGeneration, [] () {
  auto key1 = TimeWindowPartitioner::partitionKeyFor(
      "fnord-metric",
      UnixTime(1430667887 * kMicrosPerSecond),
      3600 * 4 * kMicrosPerSecond);

  auto key2 = TimeWindowPartitioner::partitionKeyFor(
      "fnord-metric",
      UnixTime(1430667880 * kMicrosPerSecond),
      3600 * 4 * kMicrosPerSecond);

  EXPECT_EQ(key1, SHA1::compute("fnord-metric\x1b\xf0\xef\x98\xaa\x05"));
  EXPECT_EQ(key1, key2);
});

