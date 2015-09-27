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
    auto v = ztrackid_decode("IPB0AA2");
    EXPECT_EQ(v, "dawanda");
  }

  {
    auto v = ztrackid_decode("ipb0aa2");
    EXPECT_EQ(v, "dawanda");
  }

  {
    auto v = ztrackid_decode("1X3P9JHTF0CE116DII8V");
    EXPECT_EQ(v, "zscale-production");
  }

  {
    auto v = ztrackid_decode("1x3p9jhtf0ce116dii8v");
    EXPECT_EQ(v, "zscale-production");
  }
});

