// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <stx/executor/ThreadPool.h>
#include <stx/executor/PosixScheduler.h>
#include <stx/thread/Wakeup.h>
#include <stx/exception.h>
#include <stx/WallClock.h>
#include <stx/UnixTime.h>
#include <stx/logging.h>
#include <stx/sysconfig.h>
#include <system_error>
#include <thread>
#include <exception>
#include <typeinfo>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

namespace stx {

#define ERROR(msg...) logError("ThreadPool", msg)

#ifndef NDEBUG
#define TRACE(msg...) logTrace("ThreadPool", msg)
#else
#define TRACE(msg...) do {} while (0)
#endif

ThreadPool::ThreadPool(std::function<void(const std::exception&)> eh)
    : ThreadPool(processorCount(), eh) {
}

ThreadPool::ThreadPool(
    size_t num_threads,
    std::function<void(const std::exception&)> eh)
    : Scheduler(eh),
      active_(true),
      threads_(),
      mutex_(),
      condition_(),
      pendingTasks_(),
      activeTasks_(0),
      activeTimers_(0),
      activeReaders_(0),
      activeWriters_(0) {

  if (num_threads < 1)
    throw std::runtime_error("Invalid argument.");

  for (size_t i = 0; i < num_threads; i++) {
    threads_.emplace_back(std::bind(&ThreadPool::work, this, i));
  }
}

ThreadPool::~ThreadPool() {
  stop();

  for (std::thread& thread: threads_) {
    thread.join();
  }
}

size_t ThreadPool::pendingCount() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return pendingTasks_.size();
}

size_t ThreadPool::activeCount() const {
  return activeTasks_;
}

void ThreadPool::wait() {
  TRACE("%p wait()", this);
  std::unique_lock<std::mutex> lock(mutex_);

  if (pendingTasks_.empty() && activeTasks_ == 0) {
    TRACE("%p wait: pending=%zu, active=%zu (immediate return)", this,
          pendingTasks_.size(), activeTasks_.load());
    return;
  }

  condition_.wait(lock, [&]() -> bool {
    TRACE("%p wait: pending=%zu, active=%zu", this, pendingTasks_.size(),
          activeTasks_.load());
    return pendingTasks_.empty() && activeTasks_.load() == 0;
  });
}

void ThreadPool::stop() {
  active_ = false;
  condition_.notify_all();
}

size_t ThreadPool::processorCount() {
#if defined(HAVE_SYSCONF) && defined(_SC_NPROCESSORS_ONLN)
  int rv = sysconf(_SC_NPROCESSORS_ONLN);
  if (rv < 0)
    throw std::system_error(errno, std::system_category());

  return rv;
#else
  return 1;
#endif
}

void ThreadPool::work(int workerId) {
  TRACE("%p worker[%d] enter", this, workerId);

  while (active_) {
    Task task;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      condition_.wait(lock, [&]() { return !pendingTasks_.empty() || !active_; });

      if (!active_)
        break;

      TRACE("%p work[%d]: task received", this, workerId);
      task = std::move(pendingTasks_.front());
      pendingTasks_.pop_front();
    }

    activeTasks_++;
    safeCall(task);
    activeTasks_--;

    // notify the potential wait() call
    condition_.notify_all();
  }

  TRACE("%p worker[%d] leave", this, workerId);
}

void ThreadPool::execute(Task task) {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    TRACE("%p execute: enqueue task & notify_all", this);
    pendingTasks_.emplace_back(std::move(task));
  }
  condition_.notify_all();
}

ThreadPool::HandleRef ThreadPool::executeOnReadable(int fd, Task task, Duration tmo, Task tcb) {
  HandleRef hr(new Handle(nullptr));
  activeReaders_++;
  execute([this, task, hr, fd] {
    PosixScheduler::waitForReadable(fd);
    safeCall([&] { hr->fire(task); });
    activeReaders_--;
  });
  return nullptr;
}

ThreadPool::HandleRef ThreadPool::executeOnWritable(int fd, Task task, Duration tmo, Task tcb) {
  HandleRef hr(new Handle(nullptr));
  activeWriters_++;
  execute([this, task, hr, fd] {
    PosixScheduler::waitForWritable(fd);
    safeCall([&] { hr->fire(task); });
    activeWriters_--;
  });
  return hr;
}

ThreadPool::HandleRef ThreadPool::executeAfter(Duration delay, Task task) {
  HandleRef hr(new Handle(nullptr));
  activeTimers_++;
  execute([this, task, hr, delay] {
    usleep(delay.microseconds());
    safeCall([&] { hr->fire(task); });
    activeTimers_--;
  });
  return hr;
}

ThreadPool::HandleRef ThreadPool::executeAt(UnixTime dt, Task task) {
  HandleRef hr(new Handle(nullptr));
  activeTimers_++;
  execute([this, task, hr, dt] {
    UnixTime now = WallClock::now();
    if (dt > now) {
      Duration delay = dt - now;
      usleep(delay.microseconds());
    }
    safeCall([&] { hr->fire(task); });
    activeTimers_--;
  });
  return hr;
}

void ThreadPool::executeOnWakeup(Task task, Wakeup* wakeup, long generation) {
  activeTimers_++;
  wakeup->onWakeup(generation, [this, task] {
    execute(task);
    activeTimers_--;
  });
}

size_t ThreadPool::timerCount() {
  return activeTimers_.load();
}

size_t ThreadPool::readerCount() {
  return activeReaders_.load();
}

size_t ThreadPool::writerCount() {
  return activeWriters_.load();
}

size_t ThreadPool::taskCount() {
  return activeTasks_.load();
}

void ThreadPool::runLoop() {
  for (;;) {
    bool cont = !taskCount()
             || !timerCount()
             || !readerCount()
             || !writerCount();

    if (!cont)
      break;

    runLoopOnce();
  }
}

void ThreadPool::runLoopOnce() {
  // in terms of this implementation, I shall decide, that
  // a ThreadPool's runLoopOnce() will block the caller until
  // there is no more task running nor pending.
  wait();
}

void ThreadPool::breakLoop() {
  condition_.notify_all();
}

std::string ThreadPool::toString() const {
  char buf[32];

  int n = snprintf(buf, sizeof(buf), "ThreadPool(%zu)@%p", threads_.size(),
                   this);

  return std::string(buf, n);
}

} // namespace stx
