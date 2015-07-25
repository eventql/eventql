// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/Signal.h>
#include <gtest/gtest.h>

using namespace cortex;

TEST(Signal, empty) {
  Signal<void(int)> s;

  ASSERT_TRUE(s.empty());
  ASSERT_EQ(0, s.size());

  s(42);
}

TEST(Signal, one) {
  Signal<void(int)> s;
  int invokationValue = 0;

  auto c = s.connect([&](int i) { invokationValue += i; });
  ASSERT_FALSE(s.empty());
  ASSERT_EQ(1, s.size());

  s(42);
  ASSERT_EQ(42, invokationValue);

  s(5);
  ASSERT_EQ(47, invokationValue);

  // remove callback
  s.disconnect(c);
  ASSERT_TRUE(s.empty());
  ASSERT_EQ(0, s.size());
}
