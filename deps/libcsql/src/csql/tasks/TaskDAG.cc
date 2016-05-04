/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/random.h>
#include <csql/tasks/TaskDAG.h>

using namespace stx;

namespace csql {

TaskDAGNode::TaskDAGNode(
    TableExpressionFactoryRef expr) :
    expr_(expr) {}

TableExpressionFactoryRef TaskDAGNode::getFactory() const {
  return expr_;
}

void TaskDAGNode::addDependency(Dependency dependency) {
  dependencies_.emplace_back(dependency);
}

const Vector<TaskDAGNode::Dependency>& TaskDAGNode::getDependencies() const {
  return dependencies_;
}

TaskID TaskDAG::addTask(RefPtr<TaskDAGNode> task) {
  auto task_id = Random::singleton()->sha1();
  const auto& task_dependecies = task->getDependencies();
  auto task_status = task_dependecies.size() > 0 ?
      TaskStatus::WAITING :
      TaskStatus::RUNNABLE;

  tasks_.emplace(task_id, task);
  task_status_.emplace(task_id, task_status);
  for (const auto& dep : task_dependecies) {
    task_deps_[dep.task_id].emplace(task_id);
  }

  return task_id;
}

RefPtr<TaskDAGNode> TaskDAG::getTask(const TaskID& task_id) const {
  auto task = tasks_.find(task_id);
  if (task == tasks_.end()) {
    RAISEF(kNotFoundError, "task not found: $0", task_id.toString());
  }

  return task->second;
}

TaskStatus TaskDAG::getTaskStatus(const TaskID& task_id) const {
  auto task = task_status_.find(task_id);
  if (task == task_status_.end()) {
    RAISEF(kNotFoundError, "task not found: $0", task_id.toString());
  }

  return task->second;
}

void TaskDAG::setTaskStatusRunnable(const TaskID& task_id) {
  auto cur_status = getTaskStatus(task_id);

  switch (cur_status) {

    case TaskStatus::WAITING:
      break;

    case TaskStatus::RUNNABLE:
      RAISE(kIllegalStateError, "illegal transition: RUNNABLE -> RUNNABLE");

    case TaskStatus::COMPLETED:
      RAISE(kIllegalStateError, "illegal transition: COMPLETED -> RUNNABLE");

  }

  task_status_[task_id] = TaskStatus::RUNNABLE;
}


void TaskDAG::setTaskStatusCompleted(const TaskID& task_id) {
  auto cur_status = getTaskStatus(task_id);

  switch (cur_status) {

    case TaskStatus::WAITING:
      RAISE(kIllegalStateError, "illegal transition: WAITING -> COMPLETED");

    case TaskStatus::RUNNABLE:
      break;

    case TaskStatus::COMPLETED:
      RAISE(kIllegalStateError, "illegal transition: COMPLETED -> COMPLETED");

  }

  task_status_[task_id] = TaskStatus::COMPLETED;

  // search for all upstream dependencies and see if they just became
  // runnable
  for (const auto& dep_task_id : getOutputTasksFor(task_id)) {
    auto dep_task = getTask(dep_task_id);
    bool dep_task_runnable = true;
    for (const auto& dep : dep_task->getDependencies()) {
      if (getTaskStatus(dep.task_id) == TaskStatus::WAITING) {
        dep_task_runnable = false;
        break;
      }
    }

    if (dep_task_runnable) {
      setTaskStatusRunnable(dep_task_id);
    }
  }
}

Set<TaskID> TaskDAG::getOutputTasksFor(const TaskID& task_id) const {
  const auto& iter = task_deps_.find(task_id);
  if (iter == task_deps_.end()) {
    return Set<TaskID>();
  } else {
    return iter->second;
  }
}

Set<TaskID> TaskDAG::getAllTasks() const {
  Set<TaskID> ids;
  for (const auto& task : task_status_) {
    ids.emplace(task.first);
  }

  return ids;
}

Set<TaskID> TaskDAG::getRunnableTasks() const {
  Set<TaskID> ids;
  for (const auto& task : task_status_) {
    if (task.second == TaskStatus::RUNNABLE) {
      ids.emplace(task.first);
    }
  }

  return ids;
}

size_t TaskDAG::getNumTasks() const {
  return tasks_.size();
}

} // namespace csql
