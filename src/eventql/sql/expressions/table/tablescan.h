/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <stdlib.h>
#include <string>
#include <vector>
#include <assert.h>
#include <eventql/sql/runtime/tablerepository.h>
#include <eventql/sql/runtime/compiler.h>
#include <eventql/sql/runtime/vm.h>
#include <eventql/sql/table_iterator.h>
#include <eventql/sql/scheduler/execution_context.h>
#include <eventql/util/exception.h>

namespace csql {

class TableScan : public TableExpression {
public:

  TableScan(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<SequentialScanNode> stmt,
      ScopedPtr<TableIterator> iter);

  ScopedPtr<ResultCursor> execute() override;

  size_t getNumColumns() const override;

protected:

  bool next(SValue* out, int out_len);

  Transaction* txn_;
  ExecutionContext* execution_context_;
  ScopedPtr<TableIterator> iter_;
  Vector<ValueExpression> select_exprs_;
  Option<ValueExpression> where_expr_;
};

} // namespace csql
