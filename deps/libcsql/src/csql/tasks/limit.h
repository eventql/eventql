/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <csql/tasks/Task.h>
#include <csql/runtime/defaultruntime.h>

namespace csql {

class Limit : public Task {
public:

  Limit(
      size_t limit,
      size_t offset,
      HashMap<TaskID, ScopedPtr<ResultCursor>> input);

  bool nextRow(SValue* out, int out_len) override;

  //bool onInputRow(
  //    const TaskID& input_id,
  //    const SValue* row,
  //    int row_len) override;

protected:
  size_t limit_;
  size_t offset_;
  ScopedPtr<ResultCursorList> input_;
  size_t counter_;
};

class LimitFactory : public TaskFactory {
public:

  LimitFactory(size_t limit, size_t offset);

  RefPtr<Task> build(
      Transaction* txn,
      HashMap<TaskID, ScopedPtr<ResultCursor>> input) const override;

protected:
  size_t limit_;
  size_t offset_;
};

}
