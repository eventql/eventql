/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/schedulers/local_scheduler.h>

using namespace stx;

namespace csql {

SchedulerFactory LocalScheduler::getFactory() {
  return [] (
      Transaction* txn,
      TaskDAG* tasks,
      SchedulerCallbacks* callbacks) -> ScopedPtr<Scheduler> {
    return mkScoped<Scheduler>(
        new LocalScheduler(txn, tasks, callbacks));
  };
}

LocalScheduler::LocalScheduler(
    Transaction* txn,
    TaskDAG* tasks,
    SchedulerCallbacks* callbacks) :
    txn_(txn),
    tasks_(tasks),
    callbacks_(callbacks) {}

ScopedPtr<ResultCursor> LocalScheduler::execute(Set<TaskID> tasks) {
  Vector<ScopedPtr<ResultCursor>> cursors;

  for (const auto& task_id : tasks) {
    cursors.emplace_back(buildInstance(task_id));
  }

  if (cursors.size() == 1) {
    return std::move(cursors[0]);
  } else {
    return mkScoped(new ResultCursorList(std::move(cursors)));
  }
}

ScopedPtr<ResultCursor> LocalScheduler::buildInstance(const TaskID& task_id) {
  auto task = tasks_->getTask(task_id);

  HashMap<TaskID, ScopedPtr<ResultCursor>> input_cursors;
  for (const auto& dep_id : tasks_->getInputTasksFor(task_id)) {
    input_cursors.emplace(dep_id, buildInstance(dep_id));
  }

  auto instance = task->getFactory()->build(txn_, std::move(input_cursors));
  return mkScoped(new TaskResultCursor(instance));
}

} // namespace csql
