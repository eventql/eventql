/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <csql/tasks/TaskFactory.h>

using namespace stx;

namespace csql {

enum TaskStatus {
  WAITING, RUNNABLE, COMPLETED
};

class TaskDAGNode : public RefCounted {
public:

  struct Dependency {
    TaskID task_id;
  };

  TaskDAGNode(TableExpressionFactoryRef expr);

  TableExpressionFactoryRef getFactory() const;

  void addDependency(Dependency dependency);
  const Vector<Dependency>& getDependencies() const;

protected:
  TableExpressionFactoryRef expr_;
  Vector<Dependency> dependencies_;
};

class TaskDAG {
public:

  TaskID addTask(RefPtr<TaskDAGNode> task);

  RefPtr<TaskDAGNode> getTask(const TaskID& task_id) const;

  TaskStatus getTaskStatus(const TaskID& task_id) const;

  void setTaskStatusRunnable(const TaskID& task_id);
  void setTaskStatusCompleted(const TaskID& task_id);

  Set<TaskID> getOutputTasksFor(const TaskID& task_id) const;
  Set<TaskID> getInputTasksFor(const TaskID& task_id) const;

  Set<TaskID> getAllTasks() const;
  Set<TaskID> getRunnableTasks() const;

  size_t getNumTasks() const;

protected:

  void findRunnableTasks();

  HashMap<TaskID, RefPtr<TaskDAGNode>> tasks_;
  HashMap<TaskID, TaskStatus> task_status_;
  HashMap<TaskID, Set<TaskID>> task_deps_;
};

} // namespace csql
