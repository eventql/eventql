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
#include "eventql/util/SHA1.h"
#include "eventql/util/test/unittest.h"

using namespace util;

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

TEST_CASE(SHA1Test, TestCompareSHA1, [] () {
  auto a = SHA1Hash::fromHexString("ffffffffffffffffffffffffffffffffffffffff");
  auto b = SHA1Hash::fromHexString("1000000000000000000000000000000000000000");
  auto c = SHA1Hash::fromHexString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

  EXPECT_TRUE(a > b);
  EXPECT_TRUE(b < a);
  EXPECT_TRUE(c > b);
  EXPECT_FALSE(c > a);
  EXPECT_FALSE(a < a);
  EXPECT_FALSE(a > a);

});
