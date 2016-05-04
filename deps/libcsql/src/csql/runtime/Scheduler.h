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
#include <csql/tasks/TaskDAG.h>
#include <csql/result_cursor.h>

using namespace stx;

namespace csql {

struct SchedulerCallbacks {
  HashMap<TaskID, Vector<RowSinkFn>> on_row;
  HashMap<TaskID, Vector<Function<void (TaskID)>>> on_complete;
  Vector<Function<void ()>> on_query_finished;
};

class Scheduler {
public:
  virtual ~Scheduler() {};
  virtual ScopedPtr<ResultCursor> execute(Set<TaskID> tasks) = 0;
};

using SchedulerFactory = Function<
    ScopedPtr<Scheduler> (
        Transaction* txn,
        TaskDAG* tasks,
        SchedulerCallbacks* callbacks)>;

} // namespace csql
