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
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/util/exception.h>
#include <eventql/util/wallclock.h>
#include <eventql/util/test/unittest.h>
#include <eventql/config/process_config.h>

using namespace eventql;
UNIT_TEST(ProcessConfigTest);

TEST_CASE(ProcessConfigTest, TestProcessConfigBuilder, [] () {
  ProcessConfigBuilder builder;
  builder.setProperty("evql", "host", "localhost");
  builder.setProperty("evql", "port", "8080");
  builder.setProperty("evql", "fuu", "bar");

  auto config = builder.getConfig();
  {
    auto p = config->getProperty("evql", "host");
    EXPECT_FALSE(p.isEmpty());
    EXPECT_EQ(p.get(), "localhost");
  }

  {
    auto p = config->getProperty("evql", "port");
    EXPECT_FALSE(p.isEmpty());
    EXPECT_EQ(p.get(), "8080");
  }

  {
    auto p = config->getProperty("evql", "fuu");
    EXPECT_FALSE(p.isEmpty());
    EXPECT_EQ(p.get(), "bar");
  }
});

