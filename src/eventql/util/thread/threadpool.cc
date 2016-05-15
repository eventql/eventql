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
#include <assert.h>
#include <memory>
#include <sys/select.h>
#include <thread>
#include <unistd.h>

#include "eventql/util/application.h"
#include "eventql/util/exception.h"
#include "eventql/util/exceptionhandler.h"
#include "eventql/util/logging.h"
#include "eventql/util/thread/threadpool.h"

using util::ExceptionHandler;

namespace util {
namespace thread {

ThreadPool::ThreadPool(
    ThreadPoolOptions opts,
    size_t max_cached_threads /* = kDefaultNumCachedThreads */) :
    ThreadPool(
        opts,
        std::unique_ptr<util::ExceptionHandler>(
            new util::CatchAndAbortExceptionHandler()),
    max_cached_threads) {}

ThreadPool::ThreadPool(
    ThreadPoolOptions opts,
    std::unique_ptr<ExceptionHandler> error_handler,
    size_t max_cached_threads /* = kDefaultNumCachedThreads */) :
    error_handler_(std::move(error_handler)),
    opts_(opts),
    max_cached_threads_(max_cached_threads),
    num_threads_(0),
    free_threads_(0) {}

void ThreadPool::run(std::function<void()> task) {
  std::unique_lock<std::mutex> l(runq_mutex_);
  runq_.push_back(task);

  if (runq_.size() > free_threads_) {
    startThread();
  }

  l.unlock();
  wakeup_.notify_one();
}

void ThreadPool::runOnReadable(std::function<void()> task, int fd) {
  run([this, task, fd] () {
    fd_set op_read, op_write;
    FD_ZERO(&op_read);
    FD_ZERO(&op_write);
    FD_SET(fd, &op_read);

    auto res = select(fd + 1, &op_read, &op_write, &op_read, NULL);

    if (res == 0) {
      RAISE(kIOError, "unexpected timeout while select()ing");
    }

    if (res == -1) {
      RAISE_ERRNO(kIOError, "select() failed");
    }

    task();
  });
}

void ThreadPool::runOnWritable(std::function<void()> task, int fd) {
  run([task, fd] () {
    fd_set op_read, op_write;
    FD_ZERO(&op_read);
    FD_ZERO(&op_write);
    FD_SET(fd, &op_write);

    auto res = select(fd + 1, &op_read, &op_write, &op_write, NULL);

    if (res == 0) {
      RAISE(kIOError, "unexpected timeout while select()ing");
    }

    if (res == -1) {
      RAISE_ERRNO(kIOError, "select() failed");
    }

    task();
  });
}

void ThreadPool::runOnWakeup(
    std::function<void()> task,
    Wakeup* wakeup,
    long generation) {

  run([task, wakeup, generation] () {
    wakeup->waitForWakeup(generation);
    task();
  });
}

void ThreadPool::startThread() {
  bool cache = false;
  if (++num_threads_ <= max_cached_threads_) {
    cache = true;
  }

  try {
    std::thread thread([this, cache] () {
      if (!opts_.thread_name.isEmpty()) {
        Application::setCurrentThreadName(opts_.thread_name.get());
      }

      do {
        try {
          std::unique_lock<std::mutex> lk(runq_mutex_);
          free_threads_++;

          while (runq_.size() == 0) {
            wakeup_.wait(lk); // FIXPAUL wait with timeout and terminate thread if no jobs for N secs
          }

          assert(runq_.size() > 0);
          auto task = runq_.front();
          runq_.pop_front();
          free_threads_--;
          lk.unlock();

          task();
        } catch (const std::exception& e) {
          this->error_handler_->onException(e);
        }
      } while (cache);


      std::unique_lock<std::mutex> lk(runq_mutex_);
      --num_threads_;
    });

    thread.detach();
  } catch (const std::exception& e) {
    this->error_handler_->onException(e);
  }
}


}
}
