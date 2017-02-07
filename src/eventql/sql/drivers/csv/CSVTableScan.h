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
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/sql/table_expression.h>
#include <eventql/sql/transaction.h>
#include <eventql/sql/drivers/csv/CSVInputStream.h>
#include <eventql/sql/runtime/ValueExpression.h>

namespace csql {
namespace backends {
namespace csv {

struct CSVTableScan : public TableExpression {
public:

  CSVTableScan(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<SequentialScanNode> stmt,
      const std::vector<std::pair<std::string, SType>>& csv_columns,
      ScopedPtr<CSVInputStream> csv);

  ReturnCode execute() override;
  ReturnCode nextBatch(SVector* columns, size_t* len) override;

  size_t getColumnCount() const override;
  SType getColumnType(size_t idx) const override;

protected:

  ReturnCode loadInputRows(size_t* row_count);

  Transaction* txn_;
  ExecutionContext* execution_context_;
  RefPtr<SequentialScanNode> stmt_;
  std::vector<std::pair<std::string, SType>> csv_columns_;
  ScopedPtr<CSVInputStream> csv_;
  std::vector<ValueExpression> select_list_;
  ValueExpression where_expr_;
  std::vector<SType> output_column_types_;
  std::vector<size_t> input_column_map_;
  std::vector<SVector> input_column_buffers_;
  VMStack vm_stack_;
};

} // namespace csv
} // namespace backends
} // namespace csql
