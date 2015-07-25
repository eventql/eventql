/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Laura Schlimmer, Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "stx/ini_parser.h"
#include "stx/test/unittest.h"

using namespace stx;

UNIT_TEST(IniParserTest);

TEST_CASE(IniParserTest, TestParseIniFile, [] () {
  bool t1 = IniParser::test();
  EXPECT_EQ(t1,false);
});
