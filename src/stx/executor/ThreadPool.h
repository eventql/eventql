// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <stx/sysconfig.h>
#include <stx/executor/Scheduler.h>
#include <stx/exceptionhandler.h>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>
#include <deque>

namespace stx {

/**
 * Standard thread-safe thread pool.
 */
class ThreadPool : public Scheduler {
 public:
  /**
   * Initializes this thread pool as many threads as CPU cores are available.
   */
  ThreadPool();

  /**
   * Initializes this thread pool.
   * @param num_threads number of threads to allocate.
   */
  explicit ThreadPool(size_t num_threads);

  /**
   * Initializes this thread pool as many threads as CPU cores are available.
   */
  explicit ThreadPool(std::unique_ptr<stx::ExceptionHandler> eh);

  /**
   * Initializes this thread pool.
   *
   * @param num_threads number of threads to allocate.
   */
  ThreadPool(size_t num_threads,
             std::unique_ptr<stx::ExceptionHandler> error_handler);

  ~ThreadPool();

  static size_t processorCount();

  /**
   * Retrieves the number of pending tasks.
   */
  size_t pendingCount() const;

  /**
   * Retrieves the number of threads currently actively running a task.
   */
  size_t activeCount() const;

  /**
   * Notifies all worker threads to stop after their current job (if any).
   */
  void stop();

  /**
   * Waits until all jobs are processed.
   */
  void wait();

  using Scheduler::executeOnReadable;
  using Scheduler::executeOnWritable;

  // overrides
  void execute(Task task) override;
  HandleRef executeAfter(Duration delay, Task task) override;
  HandleRef executeAt(UnixTime dt, Task task) override;
  HandleRef executeOnReadable(int fd, Task task, Duration tmo, Task tcb) override;
  HandleRef executeOnWritable(int fd, Task task, Duration tmo, Task tcb) override;
  void cancelFD(int fd) override;
  void executeOnWakeup(Task task, Wakeup* wakeup, long generation) override;
  size_t timerCount() override;
  size_t readerCount() override;
  size_t writerCount() override;
  size_t taskCount() override;
  void runLoop() override;
  void runLoopOnce() override;
  void breakLoop() override;
  std::string toString() const override;

  // compatibility layer to old ThreadPool API
  STX_DEPRECATED void run(std::function<void()> task);
  STX_DEPRECATED void runOnReadable(std::function<void()> task, int fd);
  STX_DEPRECATED void runOnWritable(std::function<void()> task, int fd);
  STX_DEPRECATED void runOnWakeup(std::function<void()> task,
                                  Wakeup* wakeup,
                                  long generation);

 private:
  void work(int workerId);

 private:
  bool active_;
  std::deque<std::thread> threads_;
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::deque<Task> pendingTasks_;
  std::atomic<size_t> activeTasks_;
  std::atomic<size_t> activeTimers_;
  std::atomic<size_t> activeReaders_;
  std::atomic<size_t> activeWriters_;
};

} // namespace stx
