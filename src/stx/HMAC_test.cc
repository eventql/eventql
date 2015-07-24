/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord/HMAC.h"
#include "fnord/test/unittest.h"

using namespace fnord;

UNIT_TEST(HMACTest);

TEST_CASE(HMACTest, TestComputeSHA1HMAC, [] () {
  auto sha1 = HMAC::hmac_sha1(
      Buffer(String("geheim")),
      Buffer(String("test")));

  EXPECT_EQ(sha1.toString(), "dcd7f40dbefa5875e66003d026d5c82c530292ce");
});
