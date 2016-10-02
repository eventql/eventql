/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/sql/qtree/QueryTreeNode.h>
#include <eventql/sql/transaction.h>
#include <eventql/sql/result_list.h>
#include <eventql/sql/scheduler/execution_context.h>

namespace csql {
class Runtime;

class QueryPlan : public RefCounted {
public:

  /**
   * This constructor isn't usually directly called by users but invoked through
   * Runtime::buildQueryPlan
   */
  QueryPlan(
      Transaction* txn,
      Vector<RefPtr<QueryTreeNode>> qtrees);

  /**
   * Execute one of the statements in the query plan. The statement is referenced
   * by index. The index must be in the range  [0, numStatements)
   *
   * This method returns a result cursor. The underlying query will be executed
   * incrementally as result rows are read from the cursor
   */
  ScopedPtr<ResultCursor> execute(size_t stmt_idx);

  /**
   * Execute one of the statements in the query plan. The statement is referenced
   * by index. The index must be in the range  [0, numStatements)
   *
   * This method materializes the full result list into the provided result list
   * object
   */
  void execute(size_t stmt_idx, ResultList* result_list);

  /**
   * Retruns the number of statements in the query plan
   */
  size_t numStatements() const;

  /**
   * Returns the result column list ("header") for a statement in the query plan.
   * The statement is referenced by index. The index must be in the range
   * [0, numStatements)
   */
  const Vector<String>& getStatementgetResultColumns(size_t stmt_idx);

  RefPtr<QueryTreeNode> getStatement(size_t stmt_idx) const;

  /**
   * Returns the progress of the query with index idx as a floating point value
   * between 0.0 and 1.0
   */
  double getQueryProgress(size_t query_idx) const;

  /**
   * Returns the combined progress of all queries as a floating point value
   * between 0.0 and 1.0 
   */
  double getProgress() const;
  uint64_t getTasksCount() const;
  uint64_t getTasksRunningCount() const;
  uint64_t getTasksCompletedCount() const;
  uint64_t getTasksFailedCount() const;

  /**
   * Sets a callback that will be called every time the progress changes
   */
  void setProgressCallback(Function<void()> cb);

  Transaction* getTransaction() const;

protected:
  Transaction* txn_;
  Vector<RefPtr<QueryTreeNode>> qtrees_;
  Vector<Vector<String>> statement_columns_;
  Vector<ExecutionContext> execution_contexts_;
  Function<void()> progress_callback_;
};

}
