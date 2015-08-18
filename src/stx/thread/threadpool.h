/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_THREAD_THREADPOOL_H
#define _libstx_THREAD_THREADPOOL_H
#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include "stx/thread/task.h"
#include "stx/thread/taskscheduler.h"
#include "stx/thread/wakeup.h"
#include "stx/exceptionhandler.h"

namespace stx {
namespace thread {

/**
 * A threadpool is threadsafe
 */
class ThreadPool : public TaskScheduler {
public:
  ThreadPool();

  ThreadPool(
      std::unique_ptr<stx::ExceptionHandler> error_handler);

  void run(std::function<void()> task) override;
  void runOnReadable(std::function<void()> task, int fd) override;
  void runOnWritable(std::function<void()> task, int fd) override;
  void runOnWakeup(
      std::function<void()> task,
      Wakeup* wakeup,
      long generation) override;

protected:
  void startThread();

  std::unique_ptr<stx::ExceptionHandler> error_handler_;
  std::mutex runq_mutex_;
  std::list<std::function<void()>> runq_;
  std::condition_variable wakeup_;
  std::atomic<int> free_threads_;
};

using CachedThreadPool = ThreadPool;

}
}
#endif
