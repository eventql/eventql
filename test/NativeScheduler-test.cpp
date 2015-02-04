// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/executor/NativeScheduler.h>
#include <xzero/WallClock.h>
#include <xzero/DateTime.h>
#include <gtest/gtest.h>
#include <memory>

using namespace xzero;

TEST(NativeScheduler, executeAfter_without_handle) {
  WallClock* clock = WallClock::system();
  NativeScheduler scheduler;
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

TEST(NativeScheduler, cancel_beforeRun) {
  WallClock* clock = WallClock::system();
  NativeScheduler scheduler;
  int fireCount = 0;

  auto handle = scheduler.executeAfter(TimeSpan::fromSeconds(1), [&](){
    fireCount++;
  });

  ASSERT_EQ(1, scheduler.timerCount());
  handle->cancel();
  ASSERT_EQ(0, scheduler.timerCount());
}

TEST(NativeScheduler, cancel_beforeRun2) {
  WallClock* clock = WallClock::system();
  NativeScheduler scheduler;
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
