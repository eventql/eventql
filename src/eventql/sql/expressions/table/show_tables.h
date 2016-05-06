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
#include <eventql/util/stdtypes.h>
#include <eventql/sql/qtree/ShowTablesNode.h>
#include <eventql/sql/tasks/Task.h>
#include <eventql/sql/runtime/tablerepository.h>

namespace csql {

class ShowTables : public Task {
public:

  ShowTables(Transaction* txn);

  //void onInputsReady() override;

  bool nextRow(SValue* out, int out_len) override;

protected:
  Transaction* txn_;
};

class ShowTablesFactory : public TaskFactory {
public:

  RefPtr<Task> build(
      Transaction* txn,
      HashMap<TaskID, ScopedPtr<ResultCursor>> input) const override;

};

}
