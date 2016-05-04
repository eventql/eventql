/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/HMAC.h"
#include "stx/test/unittest.h"

using namespace stx;

UNIT_TEST(HMACTest);

TEST_CASE(HMACTest, TestComputeSHA1HMAC, [] () {
  auto sha1 = HMAC::hmac_sha1(
      Buffer(String("geheim")),
      Buffer(String("test")));

  EXPECT_EQ(sha1.toString(), "dcd7f40dbefa5875e66003d026d5c82c530292ce");
});
