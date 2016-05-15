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
#include <eventql/sql/runtime/ExecutionContext.h>

using namespace util;

namespace csql {

ExecutionContext::ExecutionContext(
    TaskScheduler* sched,
    size_t max_concurrent_tasks /* = 32 */) :
    sched_(sched),
    max_concurrent_tasks_(max_concurrent_tasks),
    cancelled_(false),
    running_tasks_(1) {}

void ExecutionContext::onStatusChange(
    Function<void (const ExecutionStatus& status)> fn) {
  std::unique_lock<std::mutex> lk(mutex_);
  on_status_change_ = fn;
}

void ExecutionContext::onCancel(Function<void ()> fn) {
  std::unique_lock<std::mutex> lk(mutex_);
  on_cancel_ = fn;
}

void ExecutionContext::cancel() {
  std::unique_lock<std::mutex> lk(mutex_);

  if (on_cancel_) {
    on_cancel_();
  }

  cancelled_ = true;
}

bool ExecutionContext::isCancelled() const {
  return cancelled_;
}

void ExecutionContext::runAsync(Function<void ()> fn) {
  std::unique_lock<std::mutex> lk(mutex_);

  while (running_tasks_ >= max_concurrent_tasks_) {
    cv_.wait(lk);
  }

  sched_->run([this, fn] {
    try {
      fn();
    } catch (...) {
      std::unique_lock<std::mutex> lk(mutex_);
      --running_tasks_;
      cv_.notify_all();
      throw;
    }

    std::unique_lock<std::mutex> lk(mutex_);
    --running_tasks_;
    cv_.notify_all();
  });

  ++running_tasks_;
}

void ExecutionContext::incrNumSubtasksTotal(size_t n) {
  status_.num_subtasks_total += n;
  statusChanged();
}

void ExecutionContext::incrNumSubtasksCompleted(size_t n) {
  status_.num_subtasks_completed += n;
  statusChanged();
}

void ExecutionContext::incrNumRowsScanned(size_t n) {
  status_.num_rows_scanned += n;
  statusChanged();
}

void ExecutionContext::statusChanged() {
  // FIXPAUL rate limit...

  std::unique_lock<std::mutex> lk(mutex_);
  if (on_status_change_) {
    auto cb = on_status_change_;
    lk.unlock();
    cb(status_);
  }
}

ExecutionStatus::ExecutionStatus() :
    num_subtasks_total(1),
    num_subtasks_completed(0) {}

String ExecutionStatus::toString() const {
  return StringUtil::format(
      "$0/$1 ($2%)",
      num_subtasks_completed.load(),
      num_subtasks_total.load(),
      progress() * 100);
}

double ExecutionStatus::progress() const {
  return num_subtasks_completed / (double) num_subtasks_total;
}

Option<String> ExecutionContext::cacheDir() const {
  return cachedir_;
}

void ExecutionContext::setCacheDir(const String& cachedir) {
  cachedir_ = Some(cachedir);
}

}
