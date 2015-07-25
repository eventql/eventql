// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/executor/PosixScheduler.h>
#include <cortex-base/WallClock.h>
#include <cortex-base/DateTime.h>
#include <gtest/gtest.h>
#include <memory>

using namespace cortex;

TEST(PosixScheduler, executeAfter_without_handle) {
  WallClock* clock = WallClock::system();
  PosixScheduler scheduler;
  DateTime firedAt, start;
  int fireCount = 0;

  start = clock->get();

  scheduler.executeAfter(TimeSpan::fromMilliseconds(500), [&](){
    firedAt = clock->get();
    fireCount++;
  });

  scheduler.runLoopOnce();

  double diff = firedAt.value() - start.value();

  ASSERT_EQ(1, fireCount);
  ASSERT_NEAR(0.5, diff, 0.05);
}

TEST(PosixScheduler, cancel_beforeRun) {
  PosixScheduler scheduler;
  int fireCount = 0;

  auto handle = scheduler.executeAfter(TimeSpan::fromSeconds(1), [&](){
    fireCount++;
  });

  ASSERT_EQ(1, scheduler.timerCount());
  handle->cancel();
  ASSERT_EQ(0, scheduler.timerCount());
}

TEST(PosixScheduler, cancel_beforeRun2) {
  PosixScheduler scheduler;
  int fire1Count = 0;
  int fire2Count = 0;

  auto handle1 = scheduler.executeAfter(TimeSpan::fromSeconds(1), [&](){
    fire1Count++;
  });

  auto handle2 = scheduler.executeAfter(TimeSpan::fromMilliseconds(10), [&](){
    fire2Count++;
  });

  ASSERT_EQ(2, scheduler.timerCount());
  handle1->cancel();
  ASSERT_EQ(1, scheduler.timerCount());

  scheduler.runLoopOnce();

  ASSERT_EQ(0, fire1Count);
  ASSERT_EQ(1, fire2Count);
}
