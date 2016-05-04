/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <csql/qtree/QueryTreeNode.h>
#include <csql/tasks/TaskDAG.h>
#include <csql/runtime/Scheduler.h>

namespace csql {
class Runtime;
class ResultList;

class QueryPlan {
public:

  QueryPlan(
      Transaction* txn,
      Vector<RefPtr<QueryTreeNode>> qtrees);

  void execute();

  void setScheduler(SchedulerFactory scheduler);

  void onOutputRow(size_t stmt_idx, RowSinkFn fn);

  void storeResults(size_t stmt_idx, ResultList* result_list);

  const Vector<String>& getStatementOutputColumns(size_t stmt_idx);

  size_t numStatements() const;

  Statement* getStatement(size_t stmt_idx) const;
  RefPtr<QueryTreeNode> getStatementQTree(size_t stmt_idx) const;


protected:
  Transaction* txn_;
  Vector<RefPtr<QueryTreeNode>> qtrees_;
  Vector<TaskIDList> statement_tasks_;
  Vector<Vector<String>> statement_columns_;
  TaskDAG tasks_;
  SchedulerFactory scheduler_;
  SchedulerCallbacks callbacks_;
};

}
