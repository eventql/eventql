/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <zbase/dproc/TaskResultFuture.h>

using namespace stx;

namespace dproc {

TaskResultFuture::TaskResultFuture() : cancelled_(false) {}

Future<RefPtr<TaskRef>> TaskResultFuture::result() const {
  return promise_.future();
}

void TaskResultFuture::returnResult(RefPtr<TaskRef> result) {
  promise_.success(result);
}

void TaskResultFuture::returnError(const StandardException& e) {
  promise_.failure(e);
}

void TaskResultFuture::updateStatus(Function<void (TaskStatus* status)> fn) {
  std::unique_lock<std::mutex> lk(status_mutex_);
  fn(&status_);

  if (on_status_change_) {
    auto cb = on_status_change_;
    lk.unlock();
    cb();
  }
}

void TaskResultFuture::onStatusChange(Function<void ()> fn) {
  std::unique_lock<std::mutex> lk(status_mutex_);
  on_status_change_ = fn;
}

void TaskResultFuture::onCancel(Function<void ()> fn) {
  std::unique_lock<std::mutex> lk(status_mutex_);
  on_cancel_ = fn;
}

TaskStatus TaskResultFuture::status() const {
  std::unique_lock<std::mutex> lk(status_mutex_);
  return status_;
}

TaskStatus::TaskStatus() :
    num_subtasks_total(1),
    num_subtasks_completed(0) {}

String TaskStatus::toString() const {
  return StringUtil::format(
      "$0/$1 ($2%)",
      num_subtasks_completed,
      num_subtasks_total,
      progress() * 100);
}

double TaskStatus::progress() const {
  return num_subtasks_completed / (double) num_subtasks_total;
}

void TaskResultFuture::cancel() {
  std::unique_lock<std::mutex> lk(status_mutex_);

  if (on_cancel_) {
    on_cancel_();
  }

  cancelled_ = true;
}

bool TaskResultFuture::isCancelled() const {
  return cancelled_;
}

} // namespace dproc
