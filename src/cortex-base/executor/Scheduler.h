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
#include <cortex-base/TimeSpan.h>
#include <cortex-base/DateTime.h>
#include <cortex-base/executor/Executor.h>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>

namespace cortex {

class Wakeup;

/**
 * Interface for scheduling tasks.
 */
class CORTEX_API Scheduler : public Executor {
 public:
  /**
   * It basically supports 2 actions, fire and cancellation.
   */
  class CORTEX_API Handle {
   public:
    Handle(Task onFire, std::function<void(Handle*)> onCancel);

    /** Tests if the interest behind this handle was cancelled already. */
    bool isCancelled() const { return isCancelled_.load(); }

    /**
     * Cancels the interest, causing the callback not to be fired.
     */
    void cancel();

    /**
     * This method is invoked internally when the given intended event fired.
     *
     * Do not use this explicitely unless your code can deal with it.
     *
     * This method will not invoke the fire-callback if @c cancel() was invoked
     * already.
     */
    void fire();

   private:
    std::mutex mutex_;
    Task onFire_;
    std::function<void(Handle*)> onCancel_;
    std::atomic<bool> isCancelled_;
  };

  typedef std::shared_ptr<Handle> HandleRef;

  Scheduler(std::function<void(const std::exception&)> eh)
      : Executor(eh) {}

  /**
   * Schedules given task to be run after given delay.
   *
   * @param task the actual task to be executed.
   * @param delay the timespan to wait before actually executing the task.
   */
  virtual HandleRef executeAfter(TimeSpan delay, Task task) = 0;

  /**
   * Runs given task at given time.
   */
  virtual HandleRef executeAt(DateTime dt, Task task) = 0;

  /**
   * Runs given task when given selectable is non-blocking readable.
   */
  virtual HandleRef executeOnReadable(int fd, Task task) = 0;

  /**
   * Runs given task when given selectable is non-blocking writable.
   */
  virtual HandleRef executeOnWritable(int fd, Task task) = 0;

  /**
   * Executes @p task  when given @p wakeup triggered a wakeup event
   * for >= @p generation.
   *
   * @param task Task to invoke when the wakeup is triggered.
   * @param wakeup Wakeup object to watch
   * @param generation Generation number to match at least.
   */
  virtual void executeOnWakeup(Task task, Wakeup* wakeup, long generation) = 0;

  /**
   * Run the provided task when the wakeup handle is woken up.
   */
  void executeOnNextWakeup(std::function<void()> task, Wakeup* wakeup);

  /**
   * Run the provided task when the wakeup handle is woken up.
   */
  void executeOnFirstWakeup(std::function<void()> task, Wakeup* wakeup);

  /**
   * Retrieves the number of active timers.
   *
   * @see executeAt(DateTime dt, Task task)
   * @see executeAfter(TimeSpan ts, Task task)
   */
  virtual size_t timerCount() = 0;

  /**
   * Retrieves the number of active read-interests.
   *
   * @see executeOnReadable(int fd, Task task)
   */
  virtual size_t readerCount() = 0;

  /**
   * Retrieves the number of active write-interests.
   *
   * @see executeOnWritable(int fd, Task task)
   */
  virtual size_t writerCount() = 0;

  /**
   * Retrieves the number of pending tasks.
   *
   * @see execute(Task task)
   */
  virtual size_t taskCount() = 0;

  /**
   * Runs the event loop until no event is to be served.
   */
  virtual void runLoop() = 0;

  /**
   * Runs the event loop exactly once, possibly blocking until an event is
   * fired..
   */
  virtual void runLoopOnce() = 0;

  /**
   * Breaks loop in case it is blocking while waiting for an event.
   */
  virtual void breakLoop() = 0;

 protected:
  void safeCallEach(std::vector<HandleRef>& handles) {
    for (HandleRef& handle: handles)
      safeCall(std::bind(&Handle::fire, handle));
  }

  void safeCallEach(const std::deque<Task>& tasks) {
    for (Task task: tasks) {
      safeCall(task);
    }
  }

};

}  // namespace cortex
