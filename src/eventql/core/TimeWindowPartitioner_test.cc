/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "eventql/util/test/unittest.h"
#include "eventql/core/Partition.h"
#include "eventql/core/TimeWindowPartitioner.h"

using namespace stx;
using namespace zbase;

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

