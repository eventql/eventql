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
#include <eventql/sql/runtime/tablerepository.h>

namespace csql {

class DescribeTableStatement : public TableExpression {
public:

  DescribeTableStatement(
      Transaction* txn,
      const String& table_name);

  //void onInputsReady() override;

  ScopedPtr<ResultCursor> execute() override;


protected:

  bool next(SValue* row, size_t row_len);

  const size_t k_num_columns_ = 4;
  size_t pos_ = 0;

  Transaction* txn_;
  String table_name_;
  Vector<ColumnInfo> columns_;
};

}
