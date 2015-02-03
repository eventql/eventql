// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/executor/ThreadPool.h>
#include <xzero/logging/LogSource.h>
#include <xzero/sysconfig.h>
#include <system_error>
#include <exception>
#include <typeinfo>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

// #ifdef HAVE_PTHREAD_H
// #include <pthread.h>
// #endif

namespace xzero {

static LogSource threadPoolLogger("ThreadPool");
#define ERROR(msg...) do { threadPoolLogger.error(msg); } while (0)
#ifndef NDEBUG
#define TRACE(msg...) do { threadPoolLogger.trace(msg); } while (0)
#else
#define TRACE(msg...) do {} while (0)
#endif

ThreadPool::ThreadPool(std::function<void(const std::exception&)>&& eh)
    : ThreadPool(processorCount(), std::move(eh)) {
}

ThreadPool::ThreadPool(
    size_t num_threads,
    std::function<void(const std::exception&)>&& eh)
    : Executor(std::move(eh)),
      active_(true),
      threads_(),
      mutex_(),
      condition_(),
      pendingTasks_(),
      activeTasks_(0) {

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
    try {
      safeCall(task);
    } catch (std::exception& e) {
      ERROR("%p worker[%d] Unhandled exception %s caught. %s",
            this, workerId, typeid(e).name(), e.what());
    }
    activeTasks_--;

    // notify the potential wait() call
    condition_.notify_all();
  }

  TRACE("%p worker[%d] leave", this, workerId);
}

void ThreadPool::execute(Task&& task) {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    TRACE("%p execute: enqueue task & notify_all", this);
    pendingTasks_.emplace_back(std::move(task));
  }
  condition_.notify_all();
}

std::string ThreadPool::toString() const {
  char buf[32];

  int n = snprintf(buf, sizeof(buf), "ThreadPool(%zu)@%p", threads_.size(),
                   this);

  return std::string(buf, n);
}

} // namespace xzero
