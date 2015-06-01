/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-dproc/TaskResult.h>

namespace fnord {
namespace dproc {

Future<RefPtr<VFSFile>> TaskResult::result() const {
  return promise_.future();
}

void TaskResult::returnResult(RefPtr<VFSFile> result) {
  promise_.success(result);
}

void TaskResult::returnError(const StandardException& e) {
  promise_.failure(e);
}

void TaskResult::updateStatus(Function<void (TaskStatus* status)> fn) {
  std::unique_lock<std::mutex> lk(status_mutex_);
  fn(&status_);
  lk.unlock();
  on_status_change_();
}

void TaskResult::onStatusChange(Function<void ()> fn) {
  on_status_change_ = fn;
}

TaskStatus TaskResult::status() const {
  std::unique_lock<std::mutex> lk(status_mutex_);
  return status_;
}

TaskStatus::TaskStatus() :
    num_subtasks_total(1),
    num_subtasks_completed(0) {}

String TaskStatus::toString() const {
  return StringUtil::format(
      "$0/$1",
      num_subtasks_completed,
      num_subtasks_total);
}

} // namespace dproc
} // namespace fnord
