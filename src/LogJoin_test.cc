/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stdlib.h>
#include <stdio.h>
#include "fnord-base/exception.h"
#include "fnord-base/test/unittest.h"
#include "logjoin/LogJoinTarget.cc"

using namespace fnord;
using namespace cm;

UNIT_TEST(LogJoinTest);

TEST_CASE(LogJoinTest, Test1, [] () {
  fnord::iputs("test logjoin", 1);
  Buffer loglines_txt = FileUtil::read("src/test_loglines.txt");
  Vector<String> loglines = StringUtil::split(loglines_txt.toString(), "/n");
  TrackedSession session;

  //fnord::iputs("loglines: $0", loglines.toString());
});
