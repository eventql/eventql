/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
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

using stx::ExceptionHandler;

namespace stx {
namespace thread {

ThreadPool::ThreadPool(
    ThreadPoolOptions opts,
    size_t max_cached_threads /* = kDefaultNumCachedThreads */) :
    ThreadPool(
        opts,
        std::unique_ptr<stx::ExceptionHandler>(
            new stx::CatchAndAbortExceptionHandler()),
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
