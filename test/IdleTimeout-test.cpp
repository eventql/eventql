#include <xzero/IdleTimeout.h>
#include <xzero/DateTime.h>
#include <xzero/support/libev/LibevScheduler.h>
#include <xzero/support/libev/LibevClock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace xzero;
using namespace xzero::support;

TEST(IdleTimeout, elapsed) {
  ev::loop_ref loop = ev::default_loop(0);
  std::unique_ptr<WallClock> clock(new LibevClock(loop));
  std::unique_ptr<Scheduler> scheduler(new LibevScheduler(loop));

  DateTime beforeIdling = clock->get();
  IdleTimeout idle(clock.get(), scheduler.get());
  idle.setTimeout(TimeSpan::fromMilliseconds(50));
  idle.setCallback([](){});
  idle.activate();
  loop.run();
  DateTime afterIdling = clock->get();
  TimeSpan idlingTime = afterIdling - beforeIdling;

  ASSERT_EQ(0, idle.elapsed().totalMilliseconds());
  ASSERT_NEAR(50, idlingTime.totalMilliseconds(), 10);
}

TEST(IdleTimeout, test1) {
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

TEST(IdleTimeout, test2) {
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

TEST(IdleTimeout, touch) {
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
