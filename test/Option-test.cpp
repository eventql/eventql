// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <gtest/gtest.h>
#include <xzero/Option.h>

using xzero::Option;
using xzero::Some;
using xzero::None;

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
