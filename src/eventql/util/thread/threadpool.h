/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#ifndef _libstx_THREAD_THREADPOOL_H
#define _libstx_THREAD_THREADPOOL_H
#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include "eventql/util/thread/task.h"
#include "eventql/util/thread/taskscheduler.h"
#include "eventql/util/thread/wakeup.h"
#include "eventql/util/exceptionhandler.h"
#include "eventql/util/option.h"

namespace thread {

struct ThreadPoolOptions {
  Option<String> thread_name;
};

/**
 * A threadpool is threadsafe
 */
class ThreadPool : public TaskScheduler {
public:
  static const size_t kDefaultMaxCachedThreads = 1;

  ThreadPool(
      ThreadPoolOptions opts,
      size_t max_cached_threads = kDefaultMaxCachedThreads);

  ThreadPool(
      ThreadPoolOptions opts,
      std::unique_ptr<ExceptionHandler> error_handler,
      size_t max_cached_threads = kDefaultMaxCachedThreads);

  void run(std::function<void()> task) override;
  void runOnReadable(std::function<void()> task, int fd) override;
  void runOnWritable(std::function<void()> task, int fd) override;
  void runOnWakeup(
      std::function<void()> task,
      Wakeup* wakeup,
      long generation) override;

protected:
  void startThread();

  std::unique_ptr<ExceptionHandler> error_handler_;
  ThreadPoolOptions opts_;
  size_t max_cached_threads_;
  size_t num_threads_;
  std::atomic<int> free_threads_;
  std::mutex runq_mutex_;
  std::list<std::function<void()>> runq_;
  std::condition_variable wakeup_;
};

using CachedThreadPool = ThreadPool;

}
#endif
