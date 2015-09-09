// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <stx/executor/PosixScheduler.h>
#include <stx/WallClock.h>
#include <stx/DateTime.h>
#include <stx/RuntimeError.h>
#include <stx/logging.h>
#include <gtest/gtest.h>
#include <memory>

using namespace stx;

class SystemPipe { // {{{
 public:
  SystemPipe(int reader, int writer) {
    fds_[0] = reader;
    fds_[1] = writer;
  }

  SystemPipe() : SystemPipe(-1, -1) {
    if (pipe(fds_) < 0) {
      perror("pipe");
    }
  }

  ~SystemPipe() {
    closeEndPoint(0);
    closeEndPoint(1);
  }

  bool isValid() const noexcept { return fds_[0] != -1; }
  int readerFd() const noexcept { return fds_[0]; }
  int writerFd() const noexcept { return fds_[1]; }

  int write(const std::string& msg) {
    return ::write(writerFd(), msg.data(), msg.size());
  }

 private:
  inline void closeEndPoint(int i) {
    if (fds_[i] != -1) {
      ::close(fds_[i]);
    }
  }

 private:
  int fds_[2];
}; // }}}

TEST(PosixScheduler, executeAfter_without_handle) {
  WallClock* clock = WallClock::monotonic();
  PosixScheduler scheduler;
  DateTime start, firedAt;
  int fireCount = 0;

  scheduler.executeAfter(Duration::fromMilliseconds(500), [&](){
    firedAt = clock->get();
    fireCount++;
  });

  start = clock->get();
  firedAt = start;

  scheduler.runLoopOnce();

  float diff = firedAt.value() - start.value();

  EXPECT_EQ(1, fireCount);
  EXPECT_NEAR(0.5, diff, 0.05);
}

TEST(PosixScheduler, executeAfter_cancel_beforeRun) {
  PosixScheduler scheduler;
  int fireCount = 0;

  auto handle = scheduler.executeAfter(Duration::fromSeconds(1), [&](){
    printf("****** cancel_beforeRun: running action\n");
    fireCount++;
  });

  ASSERT_EQ(1, scheduler.timerCount());
  handle->cancel();
  EXPECT_EQ(0, scheduler.timerCount());
  EXPECT_EQ(0, fireCount);
}

TEST(PosixScheduler, executeAfter_cancel_beforeRun2) {
  PosixScheduler scheduler;
  int fire1Count = 0;
  int fire2Count = 0;

  auto handle1 = scheduler.executeAfter(Duration::fromSeconds(1), [&](){
    fire1Count++;
  });

  auto handle2 = scheduler.executeAfter(Duration::fromMilliseconds(10), [&](){
    fire2Count++;
  });

  ASSERT_EQ(2, scheduler.timerCount());
  handle1->cancel();
  ASSERT_EQ(1, scheduler.timerCount());

  scheduler.runLoopOnce();

  ASSERT_EQ(0, fire1Count);
  ASSERT_EQ(1, fire2Count);
}

TEST(PosixScheduler, executeOnReadable) {
  // executeOnReadable: test cancellation after fire
  // executeOnReadable: test fire
  // executeOnReadable: test timeout
  // executeOnReadable: test fire at the time of the timeout

  PosixScheduler sched;

  SystemPipe pipe;
  int fireCount = 0;
  int timeoutCount = 0;

  pipe.write("blurb");

  auto handle = sched.executeOnReadable(
      pipe.readerFd(),
      [&] { fireCount++; },
      Duration::Zero,
      [&] { timeoutCount++; } );

  ASSERT_EQ(0, fireCount);
  ASSERT_EQ(0, timeoutCount);

  sched.runLoopOnce();

  ASSERT_EQ(1, fireCount);
  ASSERT_EQ(0, timeoutCount);
}

TEST(PosixScheduler, executeOnReadable_timeout) {
  PosixScheduler sched;
  SystemPipe pipe;

  int fireCount = 0;
  int timeoutCount = 0;
  auto onFire = [&] { fireCount++; };
  auto onTimeout = [&] {
    printf("onTimeout!\n");
    timeoutCount++; };

  sched.executeOnReadable(pipe.readerFd(), onFire, Duration::fromMilliseconds(500), onTimeout);
  sched.runLoopOnce();

  ASSERT_EQ(0, fireCount);
  ASSERT_EQ(1, timeoutCount);
}

TEST(PosixScheduler, executeOnReadable_timeout_on_cancelled) {
  PosixScheduler sched;
  SystemPipe pipe;

  int fireCount = 0;
  int timeoutCount = 0;
  auto onFire = [&] { fireCount++; };
  auto onTimeout = [&] {
    printf("onTimeout!\n");
    timeoutCount++; };

  auto handle = sched.executeOnReadable(
      pipe.readerFd(), onFire, Duration::fromMilliseconds(500), onTimeout);

  handle->cancel();
  sched.runLoopOnce();

  ASSERT_EQ(0, fireCount);
  ASSERT_EQ(0, timeoutCount);
}

TEST(PosixScheduler, executeOnReadable_twice_on_same_fd) {
  PosixScheduler sched;
  SystemPipe pipe;

  sched.executeOnReadable(pipe.readerFd(), [] {});

  ASSERT_THROW(
      sched.executeOnReadable(pipe.readerFd(), [] {}),
      RuntimeError); // AlreadyWatchingOnResource
}

TEST(PosixScheduler, executeOnWritable) {
  PosixScheduler sched;

  SystemPipe pipe;
  int fireCount = 0;
  int timeoutCount = 0;
  const Duration timeout = Duration::fromSeconds(1);
  const auto onFire = [&]() { fireCount++; };
  const auto onTimeout = [&]() { timeoutCount++; };

  sched.executeOnWritable(pipe.writerFd(), onFire, timeout, onTimeout);

  ASSERT_EQ(0, fireCount);
  ASSERT_EQ(0, timeoutCount);

  sched.runLoopOnce();

  ASSERT_EQ(1, fireCount);
  ASSERT_EQ(0, timeoutCount);
}

TEST(PosixScheduler, waitForReadable) { // TODO
}

TEST(PosixScheduler, waitForWritable) { // TODO
}

TEST(PosixScheduler, waitForReadable_timed) { // TODO
}

TEST(PosixScheduler, waitForWritable_timed) { // TODO
}

