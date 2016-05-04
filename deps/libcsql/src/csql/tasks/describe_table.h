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
#include <csql/qtree/ShowTablesNode.h>
#include <csql/tasks/Task.h>
#include <csql/runtime/tablerepository.h>

namespace csql {

class DescribeTable : public Task {
public:

  DescribeTable(
      Transaction* txn,
      const String& table_name);

  //void onInputsReady() override;

  bool nextRow(SValue* out, int out_len) override;

protected:
  Transaction* txn_;
  String table_name_;
};

class DescribeTableFactory : public TaskFactory {
public:

  DescribeTableFactory(const String& table_name);

  RefPtr<Task> build(
      Transaction* txn,
      HashMap<TaskID, ScopedPtr<ResultCursor>> input) const override;

protected:
  String table_name_;
};

}
