// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/RefPtr.h>
#include <xzero/RefCounted.h>
#include <xzero/WallClock.h>
#include <xzero/DateTime.h>
#include <gtest/gtest.h>
#include <memory>

class RTest : public xzero::RefCounted {
 public:
  RTest() : val_(0) {}
  RTest(int x) : val_(x) {}

  int value() const { return val_; }

 private:
  int val_;
};

TEST(RefPtr, ctor0) {
  xzero::RefPtr<RTest> a;
  ASSERT_EQ(nullptr, a.get());
}

TEST(RefPtr, ctor1Nullptr) {
  xzero::RefPtr<RTest> a(nullptr);

  ASSERT_EQ(nullptr, a.get());
}

TEST(RefPtr, ctor1ptr) {
  xzero::RefPtr<RTest> a(new RTest(42));
  ASSERT_NE(nullptr, a.get());
  ASSERT_EQ(42, a->value());
  ASSERT_EQ(1, a->refCount());
}

TEST(RefPtr, ctor1Move) {
  xzero::RefPtr<RTest> a(new RTest(42));
  xzero::RefPtr<RTest> b(std::move(a));

  ASSERT_EQ(nullptr, a.get());
  ASSERT_NE(nullptr, b.get());
  ASSERT_EQ(1, b->refCount());
}

TEST(RefPtr, ctor1Copy) {
  xzero::RefPtr<RTest> a(new RTest(42));
  xzero::RefPtr<RTest> b(a);

  ASSERT_TRUE(a.get() == b.get());
  ASSERT_NE(nullptr, a.get());
  ASSERT_NE(nullptr, b.get());
  ASSERT_EQ(2, a.refCount());
}

TEST(RefPtr, assignMove) {
  xzero::RefPtr<RTest> a(new RTest(42));
  xzero::RefPtr<RTest> b;
  b = std::move(a);

  ASSERT_EQ(nullptr, a.get());
  ASSERT_NE(nullptr, b.get());
  ASSERT_EQ(1, b->refCount());
}

TEST(RefPtr, assignCopy) {
  xzero::RefPtr<RTest> a(new RTest(42));
  xzero::RefPtr<RTest> b;
  b = a;

  ASSERT_TRUE(a.get() == b.get());
  ASSERT_NE(nullptr, a.get());
  ASSERT_NE(nullptr, b.get());
  ASSERT_EQ(2, a.refCount());
}

TEST(RefPtr, dtor) {
  xzero::RefPtr<RTest> a(new RTest(42));
  {
    xzero::RefPtr<RTest> b(a);
    ASSERT_EQ(2, a.refCount());
  }
  ASSERT_EQ(1, a.refCount());
}

TEST(RefPtr, weak_as) {
  xzero::RefPtr<RTest> a(new RTest(42));
  RTest* b = a.weak_as<RTest>();
  ASSERT_EQ(1, a.refCount());
  ASSERT_TRUE(a.get() == b);
}

TEST(RefPtr, as) {
  xzero::RefPtr<RTest> a(new RTest(42));
  xzero::RefPtr<RTest> b = a.as<RTest>();
  ASSERT_EQ(2, a.refCount());
  ASSERT_TRUE(a.get() == b.get());
}

TEST(RefPtr, release) {
  xzero::RefPtr<RTest> a(new RTest(42));
  RTest* b = a.release();
  ASSERT_EQ(nullptr, a.get());
  ASSERT_EQ(0, a.refCount());
  ASSERT_EQ(1, b->refCount());
}

TEST(RefPtr, reset) {
  xzero::RefPtr<RTest> a(new RTest(42));
  a.reset();
  ASSERT_EQ(nullptr, a.get());
}

TEST(RefPtr, refEqu) {
  xzero::RefPtr<RTest> a(new RTest(42));
  xzero::RefPtr<RTest> b(new RTest(42));

  ASSERT_FALSE(a == b);
  ASSERT_TRUE(a == a);
  ASSERT_TRUE(b == b);
}

TEST(RefPtr, refNe) {
  xzero::RefPtr<RTest> a(new RTest(42));
  xzero::RefPtr<RTest> b(new RTest(42));

  ASSERT_TRUE(a != b);
  ASSERT_FALSE(a != a);
  ASSERT_FALSE(b != b);
}

TEST(RefPtr, ptrEqu) {
  xzero::RefPtr<RTest> a(new RTest(42));
  std::unique_ptr<RTest> b(new RTest(42));

  ASSERT_FALSE(a == b.get());
  ASSERT_TRUE(a == a.get());
}

TEST(RefPtr, ptrNe) {
  xzero::RefPtr<RTest> a(new RTest(42));
  std::unique_ptr<RTest> b(new RTest(42));

  ASSERT_TRUE(a != b.get());
  ASSERT_FALSE(a != a.get());
}

TEST(RefPtr, make_ref) {
  xzero::RefPtr<RTest> a = xzero::make_ref<RTest>(42);

  ASSERT_EQ(1, a.refCount());
}


