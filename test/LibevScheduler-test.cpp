// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/support/libev/LibevScheduler.h>
#include <xzero/DateTime.h>
#include <gtest/gtest.h>

using namespace xzero;
using namespace xzero::support;

TEST(LibevScheduler, DISABLED_scheduleNow) {
  ev::loop_ref loop = ev::default_loop(0);
  LibevScheduler scheduler(loop);

  ev_now_update(loop);

  DateTime fired;
  DateTime now(ev_now(loop));

  scheduler.schedule(TimeSpan::Zero, [&]() {
    fired = ev_now(loop);
  });

  loop.run();

  TimeSpan diff = fired - now;
  ASSERT_NEAR(0, diff.totalMilliseconds(), 10);
}

TEST(LibevScheduler, test1) {
  ev::loop_ref loop = ev::default_loop(0);
  LibevScheduler scheduler(loop);

  ev_now_update(loop);

  DateTime now(loop.now());
  DateTime fired = now;

  scheduler.schedule(TimeSpan::fromMilliseconds(100), [&]() {
    printf("xxxxxxxxxxxxxxxxx\n");
    ev_now_update(loop);
    fired = loop.now();
  });

  loop.run();

  printf("start: %.3f\n", now.value());
  printf("fired: %.3f\n", fired.value());

  TimeSpan diff = fired - now;

  printf("diff: %.3f\n", diff.value());

  ASSERT_NEAR(100, diff.totalMilliseconds(), 10);
}

TEST(LibevScheduler, DISABLED_testMilliseconds) {
  ev::loop_ref loop = ev::default_loop(0);
  LibevScheduler scheduler(loop);

  ev_now_update(loop);

  DateTime now(ev_now(loop));
  DateTime fired;

  scheduler.schedule(TimeSpan::fromMilliseconds(150), [&]() {
    fired = ev_now(loop);
  });

  loop.run();

  TimeSpan diff = fired - now;

  ASSERT_NEAR(150, diff.totalMilliseconds(), 10);
}

TEST(LibevScheduler, DISABLED_test2) {
  ev::loop_ref loop = ev::default_loop(0);
  LibevScheduler scheduler(loop);

  ev_now_update(loop);

  DateTime now(ev_now(loop));
  DateTime fired1;
  DateTime fired2;

  scheduler.schedule(TimeSpan::fromMilliseconds(150), [&]() {
    fired1 = ev_now(loop);
  });

  scheduler.schedule(TimeSpan::fromMilliseconds(50), [&]() {
    fired2 = ev_now(loop);
  });

  loop.run();

  TimeSpan diff1 = fired1 - now;
  TimeSpan diff2 = fired2 - now;

  ASSERT_NEAR(150, diff1.totalMilliseconds(), 10);
  ASSERT_NEAR(50,  diff2.totalMilliseconds(), 10);
}
