/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/schedulers/cluster_scheduler.h>

using namespace stx;

namespace csql {

SchedulerFactory ClusterScheduler::getFactory() {
  return [] (
      Transaction* txn,
      TaskDAG* tasks,
      SchedulerCallbacks* callbacks) -> ScopedPtr<Scheduler> {
    return mkScoped<Scheduler>(
        new ClusterScheduler(txn, tasks, callbacks));
  };
}

ClusterScheduler::ClusterScheduler(
    Transaction* txn,
    TaskDAG* tasks,
    SchedulerCallbacks* callbacks) :
    txn_(txn),
    tasks_(tasks),
    callbacks_(callbacks) {}

void ClusterScheduler::execute() {
  for (const auto& task_id : tasks_->getAllTasks()) {
    buildInstance(task_id);
  }

  for (;;) {
    auto runnables = tasks_->getRunnableTasks();
    if (runnables.empty()) {
      break;
    }

    for (const auto& runnable_id : runnables) {
      instances_[runnable_id]->onInputsReady();
      for (const auto& cb : callbacks_->on_complete[runnable_id]) {
        cb(runnable_id);
      }
      tasks_->setTaskStatusCompleted(runnable_id);
    }
  }

  for (const auto& cb : callbacks_->on_query_finished) {
    cb();
  }
}

RefPtr<Task> ClusterScheduler::buildInstance(const TaskID& task_id) {
  if (instances_.count(task_id) > 0) {
    return instances_[task_id];
  }

  auto task = tasks_->getTask(task_id);
  Vector<RowSinkFn> task_outputs;

  for (const auto& dep_id : tasks_->getOutputTasksFor(task_id)) {
    auto dep_instance = buildInstance(dep_id);
    task_outputs.emplace_back(
        std::bind(
            &Task::onInputRow,
            dep_instance.get(),
            task_id,
            std::placeholders::_1,
            std::placeholders::_2));
  }

  if (callbacks_->on_row.count(task_id) > 0) {
    for (const auto& cb : callbacks_->on_row[task_id]) {
      task_outputs.emplace_back(cb);
    }
  }

  if (instances_.count(task_id) > 0) {
    return instances_[task_id];
  }

  RowSinkFn output_fn;
  switch (task_outputs.size()) {
    case 0:
      output_fn = [] (const SValue* argv, int argc) { return true; };
      break;
    case 1:
      output_fn = task_outputs[0];
      break;
    default:
      RAISE(kNotYetImplementedError);
  }

  auto instance = task->getFactory()->build(txn_, output_fn);
  instances_.emplace(task_id, instance);
  return instance;
}

} // namespace csql
