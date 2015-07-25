// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <gtest/gtest.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/net/Cidr.h>
#include <cstdio>

using namespace cortex;

TEST(Cidr, contains) {
  Cidr cidr(IPAddress("192.168.0.0"), 24);
  IPAddress ip0("192.168.0.1");
  IPAddress ip1("192.168.1.1");

  ASSERT_TRUE(cidr.contains(ip0));
  ASSERT_FALSE(cidr.contains(ip1));
}

TEST(Cidr, equals) {
  ASSERT_EQ(Cidr("0.0.0.0", 0),  Cidr("0.0.0.0", 0));
  ASSERT_EQ(Cidr("1.2.3.4", 24), Cidr("1.2.3.4", 24));

  ASSERT_NE(Cidr("1.2.3.4", 24), Cidr("1.2.1.4", 24));
  ASSERT_NE(Cidr("1.2.3.4", 24), Cidr("1.2.3.4", 23));
}
