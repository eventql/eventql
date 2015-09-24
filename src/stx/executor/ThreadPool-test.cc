// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <stx/executor/ThreadPool.h>
#include <stx/MonotonicClock.h>
#include <stx/MonotonicTime.h>
#include <stx/test/unittest.h>
#include <unistd.h> // usleep()

using namespace stx;

UNIT_TEST(ThreadPoolTest);

TEST_CASE(ThreadPoolTest, simple, []() -> void {
  stx::ThreadPool tp(2);
  bool e1 = false;
  bool e2 = false;
  bool e3 = false;

  MonotonicTime startTime;
  MonotonicTime end1;
  MonotonicTime end2;
  MonotonicTime end3;

  startTime = MonotonicClock::now();

  tp.execute([&]() {
    usleep(Duration::fromMilliseconds(100).microseconds());
    e1 = true;
    end1 = MonotonicClock::now();
  });

  tp.execute([&]() {
    usleep(Duration::fromMilliseconds(100).microseconds());
    e2 = true;
    end2 = MonotonicClock::now();
  });

  tp.execute([&]() {
    usleep(Duration::fromMilliseconds(100).microseconds());
    e3 = true;
    end3 = MonotonicClock::now();
  });

  tp.wait(); // wait until all pending & active tasks have been executed

  EXPECT_EQ(true, e1);
  EXPECT_EQ(true, e1);
  EXPECT_EQ(true, e3); // FIXME: where is the race, why does this fail sometimes

  EXPECT_NEAR(100, (end1 - startTime).milliseconds(), 10); // executed instantly
  EXPECT_NEAR(100, (end2 - startTime).milliseconds(), 10); // executed instantly
  EXPECT_NEAR(200, (end3 - startTime).milliseconds(), 10); // executed queued
});
