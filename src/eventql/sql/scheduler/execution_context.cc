/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include "eventql/eventql.h"
#include <eventql/sql/scheduler/execution_context.h>

namespace csql {

ExecutionContext::ExecutionContext() :
    num_tasks_(0),
    num_tasks_running_(0),
    num_tasks_completed_(0) {}

void ExecutionContext::incrementNumTasks(size_t n /* = 1 */) {
  std::unique_lock<std::mutex> lk(mutex_);
  num_tasks_ += n;
}

void ExecutionContext::incrementNumTasksRunning(size_t n /* = 1 */) {
  std::unique_lock<std::mutex> lk(mutex_);
  num_tasks_running_ += n;
}

void ExecutionContext::incrementNumTasksCompleted(size_t n /* = 1 */) {
  std::unique_lock<std::mutex> lk(mutex_);
  num_tasks_completed_ += n;

  if (progress_callback_) {
    progress_callback_();
  }
}

void ExecutionContext::setProgressCallback(Function<void()> cb) {
  progress_callback_ = cb;
}

double ExecutionContext::getProgress() const {
  if (num_tasks_ == 0) {
    return 0;
  }

  if (num_tasks_completed_ > num_tasks_) {
    return 1;
  }

  return (double) num_tasks_completed_ / (double) num_tasks_;
}

} //namespace csql
