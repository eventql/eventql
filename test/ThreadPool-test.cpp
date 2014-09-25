#include <xzero/executor/ThreadPool.h>
#include <xzero/DateTime.h>
#include <chrono>
#include <gtest/gtest.h>

using xzero::DateTime;

TEST(ThreadPool, simple) {
  xzero::ThreadPool tp(2);
  bool e1 = false;
  bool e2 = false;
  bool e3 = false;

  DateTime startTime, end1, end2, end3;
  tp.execute([&]() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    e1 = true;
    end1 = DateTime();
  });
  tp.execute([&]() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    e2 = true;
    end2 = DateTime();
  });
  tp.execute([&]() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    e3 = true;
    end3 = DateTime();
  });

  tp.wait(); // wait until all pending & active tasks have been executed

  ASSERT_EQ(true, e1);
  ASSERT_EQ(true, e1);
  ASSERT_EQ(true, e3); // FIXME: where is the race, why does this fail sometimes

  ASSERT_EQ(1, (end1 - startTime).totalSeconds()); // executed instantly
  ASSERT_EQ(1, (end2 - startTime).totalSeconds()); // executed instantly
  ASSERT_EQ(2, (end3 - startTime).totalSeconds()); // executed
}
