/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#include "eventql/util/human.h"
#include "eventql/util/ISO8601.h"
#include "eventql/util/test/unittest.h"

#include "eventql/eventql.h"

UNIT_TEST(HumanTest);

TEST_CASE(HumanTest, TestParseHumanTime, [] () {
  auto t_sec = Human::parseTime("1421442618");
  EXPECT_EQ(t_sec.get().unixMicros(), 1421442618 * kMicrosPerSecond);

  auto t_milli = Human::parseTime("1421442618000");
  EXPECT_EQ(t_milli.get().unixMicros(), 1421442618 * kMicrosPerSecond);

  auto t_micro = Human::parseTime("1421442618000000");
  EXPECT_EQ(t_micro.get().unixMicros(), 1421442618 * kMicrosPerSecond);

  /*auto t_now = Human::parseTime("now");
  EXPECT_EQ(t_now.unixMicros(), UnixTime().unixMicros());*/

  auto t1 = Human::parseTime("-2hour", UnixTime(1421532360 * kMicrosPerSecond));
  EXPECT_EQ(t1.get().unixMicros(), 1421525160 * kMicrosPerSecond);

  auto t2 = Human::parseTime("-2h", UnixTime(1421532360 * kMicrosPerSecond));
  EXPECT_EQ(t2.get().unixMicros(), 1421525160 * kMicrosPerSecond);

  auto t3 = Human::parseTime("-2m", UnixTime(1421532360 * kMicrosPerSecond));
  EXPECT_EQ(t3.get().unixMicros(), 1421532240 * kMicrosPerSecond);

  auto t4 = Human::parseTime("-2minute", UnixTime(1421532360 * kMicrosPerSecond));
  EXPECT_EQ(t4.get().unixMicros(), 1421532240 * kMicrosPerSecond);

  auto t6 = Human::parseTime("-1day", UnixTime(1421532360 * kMicrosPerSecond));
  EXPECT_EQ(t6.get().unixMicros(), 1421445960 * kMicrosPerSecond);

  auto t7 = Human::parseTime("-1d", UnixTime(1421532360 * kMicrosPerSecond));
  EXPECT_EQ(t7.get().unixMicros(), 1421445960 * kMicrosPerSecond);

  auto t8 = Human::parseTime("-5s", UnixTime(1421532360 * kMicrosPerSecond));
  EXPECT_EQ(t8.get().unixMicros(), 1421532355 * kMicrosPerSecond);

  auto t9 = Human::parseTime("-5second", UnixTime(1421532360 * kMicrosPerSecond));
  EXPECT_EQ(t9.get().unixMicros(), 1421532355 * kMicrosPerSecond);

  auto t10 = Human::parseTime("-1w", UnixTime(1421532360 * kMicrosPerSecond));
  EXPECT_EQ(t10.get().unixMicros(), 1420927560 * kMicrosPerSecond);

  auto t11 = Human::parseTime("2014-01-15");
  EXPECT_EQ(t11.get().unixMicros(), 1389744000 * kMicrosPerSecond);

  auto t12 = Human::parseTime("2014-11-02");
  EXPECT_EQ(t12.get().unixMicros(), 1414886400 * kMicrosPerSecond);

  auto t13 = Human::parseTime("2014-11-02T18:23:10-01:00");
  EXPECT_EQ(t13.get().unixMicros(), 1414956190 * kMicrosPerSecond);

  auto t14 = Human::parseTime("2014-11-02T18:23:10.5-01:00");
  EXPECT_EQ(t14.get().unixMicros(), 1414956190 * kMicrosPerSecond + 500000);

  auto t15 = Human::parseTime("2014-11-02T18:23:10+00:00");
  EXPECT_EQ(t15.get().unixMicros(), 1414952590 * kMicrosPerSecond);

  auto t16 = Human::parseTime("0000-00-00 00:00:00");
  EXPECT_FALSE(t16.isEmpty());
  EXPECT_EQ(t16.get().unixMicros(), 0);

  auto t17 = Human::parseTime("0000-00-00");
  EXPECT_FALSE(t17.isEmpty());
  EXPECT_EQ(t17.get().unixMicros(), 0);


  //auto t17 = Human::parseTime("2006-09-15 20:50:06");
  //EXPECT_FALSE(t17.isEmpty());
  //EXPECT_EQ(t17.get().unixMicros(), 1158346206 * kMicrosPerSecond);
});


TEST_CASE(HumanTest, RejectInvalidISO8601TimeValues, [] () {
  {
    auto t = ISO8601::parse("0,44");
    EXPECT_TRUE(t.isEmpty());
  }

  {
    auto t = ISO8601::parse("59,89%");
    EXPECT_TRUE(t.isEmpty());
  }
});

