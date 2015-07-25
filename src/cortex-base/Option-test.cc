// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <gtest/gtest.h>
#include <cortex-base/Option.h>

using cortex::Option;
using cortex::Some;
using cortex::None;

TEST(Option, ctor0) {
  Option<int> x;

  ASSERT_TRUE(x.isNone());
}

TEST(Option, ctorNone) {
  Option<int> x = None(); // invokes the ctor Option(const None&)

  ASSERT_TRUE(x.isNone());
}

struct Movable {
  Movable(int v) : value(v) { }
  Movable(const Movable& m) : value(m.value) { }
  Movable(Movable&& m) : value(m.value) { m.value = -1; }
  Movable& operator=(Movable&& m) {
    value = m.value;
    m.value = -1;
    return *this;
  }
  int value;
};

TEST(Option, ctorMoveValue) {
  Movable i = 42;
  Option<Movable> x = std::move(i);

  ASSERT_TRUE(x.isSome());
  ASSERT_EQ(42, x->value);
  ASSERT_EQ(-1, i.value);
}

TEST(Option, moveAssign) {
  auto a = Some(Movable(42));
  auto b = Some(Movable(13));

  a = std::move(b);

  ASSERT_EQ(13, a.get().value);
  ASSERT_TRUE(b == None());
}

TEST(Option, isNone) {
  Option<int> x = None();

  ASSERT_TRUE(x.isNone());
}

TEST(Option, isSome) {
  Option<int> x = 42;
  ASSERT_FALSE(x.isNone());
  ASSERT_TRUE(x.isSome());
}

TEST(Option, operatorBool) {
  Option<int> x = 42;
  ASSERT_FALSE(!x);
  ASSERT_TRUE(x);
}

TEST(Option, operatorEqu) {
  ASSERT_TRUE(Some(42) == Some(42));
  ASSERT_FALSE(Some(13) == Some(42));
  ASSERT_FALSE(Some(13) == None());
}

TEST(Option, operatorNe) {
  ASSERT_TRUE(Some(13) != Some(42));
  ASSERT_FALSE(Some(42) != Some(42));
  ASSERT_TRUE(Some(13) != None());
}

TEST(Option, getSome) {
  Option<int> x = 42;
  ASSERT_EQ(42, x.get());
}

TEST(Option, getNone) {
  Option<int> x = None();

  ASSERT_ANY_THROW(x.get());
  ASSERT_ANY_THROW(*x);
}

TEST(Option, clear) {
  Option<int> x = 42;

  x.clear();

  ASSERT_TRUE(x.isNone());
}

TEST(Option, onSome) {
  int retrievedValue = 0;

  Some(42).onSome([&](int i) {
    retrievedValue = i;
  }).onNone([&]() {
    retrievedValue = -1;
  });

  ASSERT_EQ(42, retrievedValue);
}

TEST(Option, onNone) {
  int retrievedValue = 0;

  Option<int>().onSome([&](int i) {
    retrievedValue = i;
  }).onNone([&]() {
    retrievedValue = -1;
  });

  ASSERT_EQ(-1, retrievedValue);
}
