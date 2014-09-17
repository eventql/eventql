#include <xzero/support/libev/LibevScheduler.h>
#include <xzero/DateTime.h>
#include <gtest/gtest.h>

using namespace xzero;
using namespace xzero::support;

TEST(LibevScheduler, scheduleNow) {
  ev::loop_ref loop = ev::default_loop(0);
  LibevScheduler scheduler(loop);

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

  DateTime fired;
  DateTime now(ev_now(loop));

  scheduler.schedule(TimeSpan::fromMilliseconds(100), [&]() {
    fired = ev_now(loop);
  });

  loop.run();

  TimeSpan diff = fired - now;
  ASSERT_NEAR(100, diff.totalMilliseconds(), 10);
}

TEST(LibevScheduler, testMilliseconds) {
  ev::loop_ref loop = ev::default_loop(0);
  LibevScheduler scheduler(loop);

  DateTime now(ev_now(loop));
  DateTime fired;

  scheduler.schedule(TimeSpan::fromMilliseconds(150), [&]() {
    fired = ev_now(loop);
  });

  loop.run();

  TimeSpan diff = fired - now;

  ASSERT_NEAR(150, diff.totalMilliseconds(), 10);
}

TEST(LibevScheduler, test2) {
  ev::loop_ref loop = ev::default_loop(0);
  LibevScheduler scheduler(loop);

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
