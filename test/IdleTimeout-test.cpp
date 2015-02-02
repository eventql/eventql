// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/IdleTimeout.h>
#include <xzero/DateTime.h>
#include <xzero/support/libev/LibevScheduler.h>
#include <xzero/support/libev/LibevClock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace xzero;
using namespace xzero::support;

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

TEST(IdleTimeout, elapsed) {
  ev::loop_ref loop = ev::default_loop(0);
  ev_now_update(loop);

  std::unique_ptr<WallClock> clock(new LibevClock(loop));
  //Wrapper<WallClock> clock = WallClock::system();
  std::unique_ptr<Scheduler> scheduler(new LibevScheduler(loop));

  DateTime beforeIdling = clock->get();
  printf("before: %.4f\n", clock->get().value());

  IdleTimeout idle(clock.get(), scheduler.get());
  idle.setTimeout(TimeSpan::fromMilliseconds(50));
  idle.setCallback([](){});
  idle.activate();

  loop.run();

  DateTime afterIdling = clock->get();
  printf("after: %.4f\n", clock->get().value());

  TimeSpan idlingTime = afterIdling - beforeIdling;

  ASSERT_EQ(0, idle.elapsed().totalMilliseconds());
  ASSERT_NEAR(50, idlingTime.totalMilliseconds(), 10);
}

TEST(IdleTimeout, DISABLED_test1) {
  ev::loop_ref loop = ev::default_loop(0);
  std::unique_ptr<WallClock> clock(new LibevClock(loop));
  std::unique_ptr<Scheduler> scheduler(new LibevScheduler(loop));
  bool callbackInvoked = false;
  DateTime beforeIdling = clock->get();
  DateTime firedAt;

  IdleTimeout idle(clock.get(), scheduler.get());
  idle.setCallback([&]() {
    callbackInvoked = true;
    firedAt = clock->get();
  });

  idle.setTimeout(TimeSpan::fromMilliseconds(150));
  idle.activate();

  loop.run();

  ASSERT_EQ(true, callbackInvoked);
  ASSERT_EQ(0, idle.elapsed().totalMilliseconds());
  ASSERT_NEAR(150, (firedAt - beforeIdling).totalMilliseconds(), 10);
}

TEST(IdleTimeout, DISABLED_test2) {
  ev::loop_ref loop = ev::default_loop(0);
  std::unique_ptr<WallClock> clock(new LibevClock(loop));
  std::unique_ptr<Scheduler> scheduler(new LibevScheduler(loop));

  int callbackInvoked = 0;
  DateTime beforeIdling = clock->get();
  TimeSpan fired1;
  TimeSpan fired2;

  // idle1
  IdleTimeout idle1(clock.get(), scheduler.get());
  idle1.setCallback([&]() {
    callbackInvoked++;
    fired1 = clock->get() - beforeIdling;
  });

  idle1.setTimeout(TimeSpan::fromMilliseconds(150));
  idle1.activate();

  // idle2
  IdleTimeout idle2(clock.get(), scheduler.get());
  idle2.setCallback([&]() {
    callbackInvoked++;
    fired2 = clock->get() - beforeIdling;
  });

  idle2.setTimeout(TimeSpan::fromMilliseconds(125));
  idle2.activate();

  loop.run();

  ASSERT_EQ(2, callbackInvoked);
  ASSERT_NEAR(150, fired1.totalMilliseconds(), 10);
  ASSERT_NEAR(125, fired2.totalMilliseconds(), 10);
}

TEST(IdleTimeout, DISABLED_touch) {
  ev::loop_ref loop = ev::default_loop(0);
  std::unique_ptr<WallClock> clock(new LibevClock(loop));
  std::unique_ptr<Scheduler> scheduler(new LibevScheduler(loop));

  int callbackInvoked = 0;
  DateTime beforeIdling = clock->get();
  TimeSpan fired1;
  TimeSpan fired2;

  // idle1
  IdleTimeout idle1(clock.get(), scheduler.get());
  idle1.setCallback([&]() {
    callbackInvoked++;
    fired1 = clock->get() - beforeIdling;
  });

  idle1.setTimeout(TimeSpan::fromMilliseconds(150));
  idle1.activate();

  // idle2
  IdleTimeout idle2(clock.get(), scheduler.get());
  idle2.setCallback([&]() {
    callbackInvoked++;
    fired2 = clock->get() - beforeIdling;
    idle1.touch();
  });

  idle2.setTimeout(TimeSpan::fromMilliseconds(100));
  idle2.activate();

  loop.run();

  ASSERT_EQ(2, callbackInvoked);
  ASSERT_NEAR(100, fired2.totalMilliseconds(), 10);
  ASSERT_NEAR(250, fired1.totalMilliseconds(), 10);
}
