/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <stx/SHA1.h>
#include <csql/svalue.h>
#include <csql/runtime/ExecutionContext.h>
#include <csql/runtime/Statement.h>
#include <csql/runtime/rowsink.h>
#include <csql/tasks/TaskID.h>
#include <csql/tasks/Task.h>

using namespace stx;

namespace csql {

typedef Function<bool (const SValue* argv, int argc)> RowSinkFn;

class ResultCursor {
public:

  /**
   * Fetch the next row from the cursor. Returns true if a row was returned
   * into the provided storage and false if the last row of the query has been
   * read (EOF). If this method returns false the provided storage will not
   * be changed.
   *
   * This method will block until the next row is available. Use the polling/
   * callback interface below if you need async execution.
   */
  virtual bool next(SValue* row, int row_len) = 0;

  /**
   * Returns true if a call to next would not block and false if such a call
   * would block
   */
  virtual bool poll() {
    return true;
  }

  /**
   * Wait for the next row. Calls the provided callback exactly one time once the
   * new row becomes available. The callback may be executed in the current
   * or any other thread.
   */
  virtual void wait(Function<void ()> callback) {
    callback();
  }

};

class ResultCursorList : public ResultCursor {
public:

  ResultCursorList(Vector<ScopedPtr<ResultCursor>> cursors);
  ResultCursorList(HashMap<TaskID, ScopedPtr<ResultCursor>> cursors);

  bool next(SValue* row, int row_len) override;

protected:
  Vector<ScopedPtr<ResultCursor>> cursors_;
};

class TaskResultCursor : public ResultCursor {
public:

  TaskResultCursor(RefPtr<Task> task);

  bool next(SValue* row, int row_len) override;

protected:
  RefPtr<Task> task_;
};

}
