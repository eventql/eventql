/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/SHA1.h"
#include "stx/test/unittest.h"

using namespace stx;

UNIT_TEST(SHA1Test);

TEST_CASE(SHA1Test, TestComputeSHA1, [] () {
  EXPECT_EQ(
      SHA1::compute(String("1")).toString(),
      "356a192b7913b04c54574d18c28d46e6395428ab");

  EXPECT_EQ(
      SHA1::compute(String("2")).toString(),
      "da4b9237bacccdf19c0760cab7aec4a8359010b0");

  Set<SHA1Hash> set;
  for (int i = 0; i < 100000; ++i) {
    set.emplace(SHA1::compute(StringUtil::toString(i)));
  }

  EXPECT_EQ(set.size(), 100000);

  for (int i = 0; i < 100000; ++i) {
    EXPECT_EQ(set.count(SHA1::compute(StringUtil::toString(i))), 1);
  }
});
