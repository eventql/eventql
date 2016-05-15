/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _libstx_THREAD_FIXEDSIZETHREADPOOL_H
#define _libstx_THREAD_FIXEDSIZETHREADPOOL_H
#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <thread>
#include "eventql/util/thread/task.h"
#include "eventql/util/thread/queue.h"
#include "eventql/util/thread/taskscheduler.h"
#include "eventql/util/thread/wakeup.h"
#include "eventql/util/thread/threadpool.h"
#include "eventql/util/exceptionhandler.h"

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
      ThreadPoolOptions opts,
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
      ThreadPoolOptions opts,
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
  ThreadPoolOptions opts_;
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
