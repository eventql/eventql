/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_DPROC_TASKRESULT_H
#define _FNORD_DPROC_TASKRESULT_H
#include <fnord-base/stdtypes.h>
#include <fnord-dproc/Task.h>

namespace fnord {
namespace dproc {

struct TaskStatus {
  TaskStatus();

  size_t num_subtasks_total;
  size_t num_subtasks_completed;

  String toString() const;
  double progress() const;
};

class TaskResultFuture : public RefCounted {
public:

  Future<RefPtr<Task>> result() const;

  void returnResult(RefPtr<Task> result);
  void returnError(const StandardException& e);

  void updateStatus(Function<void (TaskStatus* status)> fn);
  void onStatusChange(Function<void ()> fn);
  TaskStatus status() const;

protected:
  TaskStatus status_;
  mutable std::mutex status_mutex_;
  Function<void ()> on_status_change_;
  Promise<RefPtr<Task>> promise_;
};

} // namespace dproc
} // namespace fnord

#endif
