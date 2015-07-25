// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <cortex-base/executor/Scheduler.h>
#include <sys/select.h>
#include <list>
#include <deque>
#include <mutex>

namespace cortex {

class WallClock;

class CORTEX_API PosixScheduler : public Scheduler {
 public:
  PosixScheduler(
      std::function<void(const std::exception&)> errorLogger,
      WallClock* clock,
      std::function<void()> preInvoke,
      std::function<void()> postInvoke);

  explicit PosixScheduler(
      std::function<void(const std::exception&)> errorLogger,
      WallClock* clock);

  PosixScheduler();

  ~PosixScheduler();

  void execute(Task task) override;
  std::string toString() const override;
  HandleRef executeAfter(TimeSpan delay, Task task) override;
  HandleRef executeAt(DateTime dt, Task task) override;
  HandleRef executeOnReadable(int fd, Task task) override;
  HandleRef executeOnWritable(int fd, Task task) override;
  void executeOnWakeup(Task task, Wakeup* wakeup, long generation) override;
  size_t timerCount() override;
  size_t readerCount() override;
  size_t writerCount() override;
  size_t taskCount() override;
  void runLoop() override;
  void runLoopOnce() override;
  void breakLoop() override;

  /**
   * Waits at most @p timeout for @p fd to become readable without blocking.
   */
  static void waitForReadable(int fd, TimeSpan timeout);

  /**
   * Waits until given @p fd becomes readable without blocking.
   */
  static void waitForReadable(int fd);

  /**
   * Waits at most @p timeout for @p fd to become writable without blocking.
   */
  static void waitForWritable(int fd, TimeSpan timeout);

  /**
   * Waits until given @p fd becomes writable without blocking.
   */
  static void waitForWritable(int fd);

 protected:
  void removeFromTimersList(Handle* handle);
  HandleRef insertIntoTimersList(DateTime dt, HandleRef handle);
  void collectTimeouts(std::vector<HandleRef>* result);

 private:
  WallClock* clock_;
  std::mutex lock_;
  int wakeupPipe_[2];

  Task onPreInvokePending_;
  Task onPostInvokePending_;

  std::deque<Task> tasks_;
  std::list<std::pair<int, HandleRef>> readers_;
  std::list<std::pair<int, HandleRef>> writers_;

  struct Timer {
    DateTime when;
    HandleRef handle;
  };
  std::list<Timer> timers_;
};

} // namespace cortex
