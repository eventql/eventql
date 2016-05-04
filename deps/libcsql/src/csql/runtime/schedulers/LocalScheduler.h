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
#include <csql/runtime/Scheduler.h>

using namespace stx;

namespace csql {
class Transaction;

class LocalScheduler : public Scheduler {
public:

  static SchedulerFactory getFactory();

  LocalScheduler(
      Transaction* txn,
      TaskDAG* tasks,
      SchedulerCallbacks* callbacks);

  void execute() override;

protected:

  RefPtr<Task> buildInstance(const TaskID& task_id);

  Transaction* txn_;
  TaskDAG* tasks_;
  SchedulerCallbacks* callbacks_;
  HashMap<TaskID, RefPtr<Task>> instances_;
};

} // namespace csql
