// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <stx/executor/PosixScheduler.h>
#include <stx/MonotonicTime.h>
#include <stx/MonotonicClock.h>
#include <stx/application.h>
#include <stx/exception.h>
#include <stx/logging.h>
#include <stx/test/unittest.h>
#include <memory>

#include <fcntl.h>
#include <unistd.h>

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

static stx::test::UnitTest PosixSchedulerTest("PosixSchedulerTest");
int main() {
  auto& t = PosixSchedulerTest;
  return t.run();
}

TEST_INITIALIZER(PosixSchedulerTest, logging, []() {
  Application::logToStderr(LogLevel::kTrace);
});

TEST_CASE(PosixSchedulerTest, executeAfter_without_handle, [] () {
  PosixScheduler scheduler;
  MonotonicTime start;
  MonotonicTime firedAt;
  int fireCount = 0;

  scheduler.executeAfter(Duration::fromMilliseconds(50), [&](){
    firedAt = MonotonicClock::now();
    fireCount++;
  });

  start = MonotonicClock::now();
  firedAt = start;

  scheduler.runLoopOnce();

  Duration diff = firedAt - start;

  EXPECT_EQ(1, fireCount);
  EXPECT_NEAR(50, diff.milliseconds(), 10);
});

TEST_CASE(PosixSchedulerTest, executeAfter_cancel_beforeRun, [] () {
  PosixScheduler scheduler;
  int fireCount = 0;

  auto handle = scheduler.executeAfter(Duration::fromSeconds(1), [&](){
    printf("****** cancel_beforeRun: running action\n");
    fireCount++;
  });

  EXPECT_EQ(1, scheduler.timerCount());
  handle->cancel();
  EXPECT_EQ(0, scheduler.timerCount());
  EXPECT_EQ(0, fireCount);
});

TEST_CASE(PosixSchedulerTest, executeAfter_cancel_beforeRun2, [] () {
  PosixScheduler scheduler;
  int fire1Count = 0;
  int fire2Count = 0;

  auto handle1 = scheduler.executeAfter(Duration::fromSeconds(1), [&](){
    fire1Count++;
  });

  auto handle2 = scheduler.executeAfter(Duration::fromMilliseconds(10), [&](){
    fire2Count++;
  });

  EXPECT_EQ(2, scheduler.timerCount());
  handle1->cancel();
  EXPECT_EQ(1, scheduler.timerCount());

  scheduler.runLoopOnce();

  EXPECT_EQ(0, fire1Count);
  EXPECT_EQ(1, fire2Count);
});

TEST_CASE(PosixSchedulerTest, executeOnReadable, [] () {
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

  EXPECT_EQ(0, fireCount);
  EXPECT_EQ(0, timeoutCount);

  sched.runLoopOnce();

  EXPECT_EQ(1, fireCount);
  EXPECT_EQ(0, timeoutCount);
});

TEST_CASE(PosixSchedulerTest, executeOnReadable_timeout, [] () {
  PosixScheduler sched;
  SystemPipe pipe;

  int fireCount = 0;
  int timeoutCount = 0;
  auto onFire = [&] { fireCount++; };
  auto onTimeout = [&] { timeoutCount++; };

  sched.executeOnReadable(pipe.readerFd(), onFire, Duration::fromMilliseconds(500), onTimeout);
  sched.runLoopOnce();

  EXPECT_EQ(0, fireCount);
  EXPECT_EQ(1, timeoutCount);
});

TEST_CASE(PosixSchedulerTest, executeOnReadable_timeout_on_cancelled, [] () {
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

  EXPECT_EQ(0, fireCount);
  EXPECT_EQ(0, timeoutCount);
});

TEST_CASE(PosixSchedulerTest, executeOnReadable_twice_on_same_fd, [] () {
  PosixScheduler sched;
  SystemPipe pipe;

  sched.executeOnReadable(pipe.readerFd(), [] () {});

  EXPECT_EXCEPTION("Already watching on resource", [&]() {
    sched.executeOnReadable(pipe.readerFd(), [] () {});
  });

  // FIXME
  // EXPECT_THROW(
  //     sched.executeOnReadable(pipe.readerFd(), [] () {}),
  //     AlreadyWatchingOnResource);
});

TEST_CASE(PosixSchedulerTest, executeOnWritable, [] () {
  PosixScheduler sched;

  SystemPipe pipe;
  int fireCount = 0;
  int timeoutCount = 0;
  const Duration timeout = Duration::fromSeconds(1);
  const auto onFire = [&]() { fireCount++; };
  const auto onTimeout = [&]() { timeoutCount++; };

  sched.executeOnWritable(pipe.writerFd(), onFire, timeout, onTimeout);

  EXPECT_EQ(0, fireCount);
  EXPECT_EQ(0, timeoutCount);

  sched.runLoopOnce();

  EXPECT_EQ(1, fireCount);
  EXPECT_EQ(0, timeoutCount);
});

// TEST_CASE(PosixSchedulerTest, waitForReadable, [] () { // TODO
// });
// 
// TEST_CASE(PosixSchedulerTest, waitForWritable, [] () { // TODO
// });
// 
// TEST_CASE(PosixSchedulerTest, waitForReadable_timed, [] () { // TODO
// });
// 
// TEST_CASE(PosixSchedulerTest, waitForWritable_timed, [] () { // TODO
// });
