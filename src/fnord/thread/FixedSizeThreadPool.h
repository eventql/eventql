/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_THREAD_FIXEDSIZETHREADPOOL_H
#define _FNORDMETRIC_THREAD_FIXEDSIZETHREADPOOL_H
#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <thread>
#include "fnord-base/thread/task.h"
#include "fnord-base/thread/queue.h"
#include "fnord-base/thread/taskscheduler.h"
#include "fnord-base/thread/wakeup.h"
#include "fnord-base/exceptionhandler.h"

namespace fnord {
namespace thread {

/**
 * A threadpool is threadsafe
 */
class FixedSizeThreadPool : public TaskScheduler {
public:

  FixedSizeThreadPool(size_t nthreads);

  FixedSizeThreadPool(
      size_t nthreads,
      std::unique_ptr<fnord::ExceptionHandler> error_handler);

  void start();
  void stop();

  void run(std::function<void()> task) override;
  void runOnReadable(std::function<void()> task, int fd) override;
  void runOnWritable(std::function<void()> task, int fd) override;
  void runOnWakeup(
      std::function<void()> task,
      Wakeup* wakeup,
      long generation) override;

protected:

  size_t nthreads_;
  std::unique_ptr<fnord::ExceptionHandler> error_handler_;
  Queue<std::function<void()>> queue_;
  Vector<std::thread> threads_;
};

}
}
#endif
