// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/IdleTimeout.h>
#include <xzero/DateTime.h>
#include <xzero/executor/NativeScheduler.h>
#include <xzero/WallClock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace xzero;

template<typename T> class Wrapper { // {{{
 public:
  Wrapper(T* v) : value_{v} {}

  const T& operator*() const { return *value_; }
  T& operator*() { return *value_; }

  T* get() { return value_; }
  const T* get() const { return value_; }

  T* operator->() { return value_; }
  const T* operator->() const { return value_; }

 private:
  T* value_;
}; // }}}

TEST(NativeScheduler, executeAfter_with_handle) {
  WallClock* clock = WallClock::system();
  NativeScheduler scheduler;
  DateTime firedAt, start;
  int fireCount = 0;

  start = clock->get();

  printf("before executeAfter(): %s\n", clock->get().http_str().str().c_str());
  auto handle = scheduler.executeAfter(TimeSpan::fromMilliseconds(1000), [&](){
    printf("fired at: %s\n", clock->get().http_str().str().c_str());
    firedAt = clock->get();
    fireCount++;
  });

  TimeSpan diff = firedAt - start;
  unsigned long diffms = diff.totalMilliseconds();

  scheduler.runLoopOnce();
  printf("diff: %lu\n", diffms);
  printf("after runLoopOnce(): %s\n", clock->get().http_str().str().c_str());

  ASSERT_EQ(1, fireCount);
  ASSERT_NEAR(1000, diffms, 10);
}

TEST(NativeScheduler, cancelBeforeRun) {
  WallClock* clock = WallClock::system();
  NativeScheduler scheduler;
  int fireCount = 0;

  auto handle = scheduler.executeAfter(TimeSpan::fromSeconds(1), [&](){
    fireCount++;
  });
  handle->cancel();

  scheduler.runLoopOnce();

  ASSERT_EQ(0, fireCount);
}
