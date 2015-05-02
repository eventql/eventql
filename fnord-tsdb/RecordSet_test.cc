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
#include "fnord-base/test/unittest.h"
#include "fnord-tsdb/RecordSet.h"

using namespace fnord;
using namespace fnord::tsdb;

UNIT_TEST(RecordSetTest);

TEST_CASE(RecordSetTest, TestAddRowToEmptySet, [] () {
  RecordSet recset(nullptr, "/tmp/__fnord_testrecset");
});
