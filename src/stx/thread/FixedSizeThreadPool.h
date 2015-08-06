/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_THREAD_FIXEDSIZETHREADPOOL_H
#define _libstx_THREAD_FIXEDSIZETHREADPOOL_H
#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <thread>
#include "stx/thread/task.h"
#include "stx/thread/queue.h"
#include "stx/thread/taskscheduler.h"
#include "stx/thread/wakeup.h"
#include "stx/exceptionhandler.h"

namespace stx {
namespace thread {

/**
 * A threadpool is threadsafe
 */
class FixedSizeThreadPool : public TaskScheduler {
public:

  /**
   * Construct a new fixed size thread pool. If maxqueuelen is set and the
   * queue is full, the the run() operation will block if the block param was
   * set to true and throw an exception otherwise. Default queue size is
   * unbounded
   *
   * @param nthreads number of threads to run
   * @param maxqueuelen max queue len. default is -1 == unbounded
   * @param true=block if the queue is full, false=throw an exception
   */
  FixedSizeThreadPool(
      size_t nthreads,
      size_t maxqueuelen = -1,
      bool block = true);

  /**
   * Construct a new fixed size thread pool. If maxqueuelen is set and the
   * queue is full, the the run() operation will block if the block param was
   * set to true and throw an exception otherwise. Default queue size is
   * unbounded
   *
   * @param nthreads number of threads to run
   * @param error_handler the exception handler to call for unhandled errors
   * @param maxqueuelen max queue len. default is -1 == unbounded
   * @param true=block if the queue is full, false=throw an exception
   */
  FixedSizeThreadPool(
      size_t nthreads,
      std::unique_ptr<stx::ExceptionHandler> error_handler,
      size_t maxqueuelen = -1,
      bool block = true);

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
  std::unique_ptr<stx::ExceptionHandler> error_handler_;
  Queue<std::function<void()>> queue_;
  bool block_;
  std::atomic<bool> running_;
  Vector<std::thread> threads_;
};

}
}
#endif
