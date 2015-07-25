// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/RefPtr.h>
#include <cortex-base/RefCounted.h>
#include <cortex-base/WallClock.h>
#include <cortex-base/DateTime.h>
#include <gtest/gtest.h>
#include <memory>

class RTest : public cortex::RefCounted {
 public:
  RTest() : val_(0) {}
  RTest(int x) : val_(x) {}

  int value() const { return val_; }

 private:
  int val_;
};

TEST(RefPtr, ctor0) {
  cortex::RefPtr<RTest> a;
  ASSERT_EQ(nullptr, a.get());
}

TEST(RefPtr, ctor1Nullptr) {
  cortex::RefPtr<RTest> a(nullptr);

  ASSERT_EQ(nullptr, a.get());
}

TEST(RefPtr, ctor1ptr) {
  cortex::RefPtr<RTest> a(new RTest(42));
  ASSERT_NE(nullptr, a.get());
  ASSERT_EQ(42, a->value());
  ASSERT_EQ(1, a->refCount());
}

TEST(RefPtr, ctor1Move) {
  cortex::RefPtr<RTest> a(new RTest(42));
  cortex::RefPtr<RTest> b(std::move(a));

  ASSERT_EQ(nullptr, a.get());
  ASSERT_NE(nullptr, b.get());
  ASSERT_EQ(1, b->refCount());
}

TEST(RefPtr, ctor1Copy) {
  cortex::RefPtr<RTest> a(new RTest(42));
  cortex::RefPtr<RTest> b(a);

  ASSERT_TRUE(a.get() == b.get());
  ASSERT_NE(nullptr, a.get());
  ASSERT_NE(nullptr, b.get());
  ASSERT_EQ(2, a.refCount());
}

TEST(RefPtr, assignMove) {
  cortex::RefPtr<RTest> a(new RTest(42));
  cortex::RefPtr<RTest> b;
  b = std::move(a);

  ASSERT_EQ(nullptr, a.get());
  ASSERT_NE(nullptr, b.get());
  ASSERT_EQ(1, b->refCount());
}

TEST(RefPtr, assignCopy) {
  cortex::RefPtr<RTest> a(new RTest(42));
  cortex::RefPtr<RTest> b;
  b = a;

  ASSERT_TRUE(a.get() == b.get());
  ASSERT_NE(nullptr, a.get());
  ASSERT_NE(nullptr, b.get());
  ASSERT_EQ(2, a.refCount());
}

TEST(RefPtr, dtor) {
  cortex::RefPtr<RTest> a(new RTest(42));
  {
    cortex::RefPtr<RTest> b(a);
    ASSERT_EQ(2, a.refCount());
  }
  ASSERT_EQ(1, a.refCount());
}

TEST(RefPtr, weak_as) {
  cortex::RefPtr<RTest> a(new RTest(42));
  RTest* b = a.weak_as<RTest>();
  ASSERT_EQ(1, a.refCount());
  ASSERT_TRUE(a.get() == b);
}

TEST(RefPtr, as) {
  cortex::RefPtr<RTest> a(new RTest(42));
  cortex::RefPtr<RTest> b = a.as<RTest>();
  ASSERT_EQ(2, a.refCount());
  ASSERT_TRUE(a.get() == b.get());
}

TEST(RefPtr, release) {
  cortex::RefPtr<RTest> a(new RTest(42));
  RTest* b = a.release();
  ASSERT_EQ(nullptr, a.get());
  ASSERT_EQ(0, a.refCount());
  ASSERT_EQ(1, b->refCount());
}

TEST(RefPtr, reset) {
  cortex::RefPtr<RTest> a(new RTest(42));
  a.reset();
  ASSERT_EQ(nullptr, a.get());
}

TEST(RefPtr, refEqu) {
  cortex::RefPtr<RTest> a(new RTest(42));
  cortex::RefPtr<RTest> b(new RTest(42));

  ASSERT_FALSE(a == b);
  ASSERT_TRUE(a == a);
  ASSERT_TRUE(b == b);
}

TEST(RefPtr, refNe) {
  cortex::RefPtr<RTest> a(new RTest(42));
  cortex::RefPtr<RTest> b(new RTest(42));

  ASSERT_TRUE(a != b);
  ASSERT_FALSE(a != a);
  ASSERT_FALSE(b != b);
}

TEST(RefPtr, ptrEqu) {
  cortex::RefPtr<RTest> a(new RTest(42));
  std::unique_ptr<RTest> b(new RTest(42));

  ASSERT_FALSE(a == b.get());
  ASSERT_TRUE(a == a.get());
}

TEST(RefPtr, ptrNe) {
  cortex::RefPtr<RTest> a(new RTest(42));
  std::unique_ptr<RTest> b(new RTest(42));

  ASSERT_TRUE(a != b.get());
  ASSERT_FALSE(a != a.get());
}

TEST(RefPtr, make_ref) {
  cortex::RefPtr<RTest> a = cortex::make_ref<RTest>(42);

  ASSERT_EQ(1, a.refCount());
}


