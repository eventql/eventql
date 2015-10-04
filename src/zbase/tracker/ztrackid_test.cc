/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/test/unittest.h"
#include "zbase/tracker/ztrackid.h"

using namespace stx;
using namespace zbase;

UNIT_TEST(ZTrackIDTest);

TEST_CASE(ZTrackIDTest, TestDecode, [] () {
  {
    auto v = ztrackid_decode("5C4RPVX58H04B");
    EXPECT_EQ(v, "dawanda");
  }

  {
    auto v = ztrackid_decode("5c4rpvx58h04b");
    EXPECT_EQ(v, "dawanda");
  }

  {
    auto v = ztrackid_decode("48CF7E4RUKXUX48CF7E4QS7HI85C4RPW6WV8U98");
    EXPECT_EQ(v, "000000-production");
  }

  {
    auto v = ztrackid_decode("48cf7e4rukxux48cf7e4qs7hi85c4rpw6wv8u98");
    EXPECT_EQ(v, "000000-production");
  }

  {
    auto v = ztrackid_decode("78KJCXEC8Q8TU5C4RPW6WV8U98");
    EXPECT_EQ(v, "zscale-production");
  }

  {
    auto v = ztrackid_decode("78kjcxec8q8tu5c4rpw6wv8u98");
    EXPECT_EQ(v, "zscale-production");
  }
});

