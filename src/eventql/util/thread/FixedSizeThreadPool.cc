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
#include "eventql/util/thread/FixedSizeThreadPool.h"
#include "eventql/util/application.h"

namespace util {
namespace thread {

FixedSizeThreadPool::FixedSizeThreadPool(
    ThreadPoolOptions opts,
    size_t nthreads,
    size_t maxqueuelen /* = -1 */,
    bool block /* = true */) :
    FixedSizeThreadPool(
        opts,
        nthreads,
        std::unique_ptr<util::ExceptionHandler>(
            new util::CatchAndAbortExceptionHandler())) {}

FixedSizeThreadPool::FixedSizeThreadPool(
    ThreadPoolOptions opts,
    size_t nthreads,
    std::unique_ptr<ExceptionHandler> error_handler,
    size_t maxqueuelen /* = -1 */,
    bool block /* = true */) :
    opts_(opts),
    nthreads_(nthreads),
    error_handler_(std::move(error_handler)),
    queue_(maxqueuelen),
    block_(block) {}

void FixedSizeThreadPool::start() {
  running_ = true;

  for (int i = 0; i < nthreads_; ++i) {
    threads_.emplace_back([this] () {
      if (!opts_.thread_name.isEmpty()) {
        Application::setCurrentThreadName(opts_.thread_name.get());
      }

      while (running_.load()) {
        auto job = queue_.interruptiblePop();

        try {
          if (!job.isEmpty()) {
            job.get()();
          }
        } catch (const std::exception& e) {
          error_handler_->onException(e);
        }
      }
    });
  }
}

void FixedSizeThreadPool::stop() {
  queue_.waitUntilEmpty();
  running_ = false;
  queue_.wakeup();

  for (auto& t : threads_) {
    t.join();
  }
}

void FixedSizeThreadPool::run(std::function<void()> task) {
  queue_.insert(task, block_);
}

void FixedSizeThreadPool::runOnReadable(std::function<void()> task, int fd) {
  RAISE(
      kNotImplementedError,
      "not suppported: FixedSizeThreadPool::runOnReadable");
}

void FixedSizeThreadPool::runOnWritable(std::function<void()> task, int fd) {
  RAISE(
      kNotImplementedError,
      "not suppported: FixedSizeThreadPool::runOnWritable");
}

void FixedSizeThreadPool::runOnWakeup(
    std::function<void()> task,
    Wakeup* wakeup,
    long generation) {
  RAISE(
      kNotImplementedError,
      "not suppported: FixedSizeThreadPool::runOnWakeup");
}

}
}
