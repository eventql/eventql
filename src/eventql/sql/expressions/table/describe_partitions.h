/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
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
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/sql/qtree/ShowTablesNode.h>
#include <eventql/sql/runtime/tablerepository.h>

namespace csql {

class DescribePartitionsExpression : public TableExpression {
public:

  static const size_t kNumColumns = 5;

  DescribePartitionsExpression(
      Transaction* txn,
      const String& table_name);

  ReturnCode execute() override;
  ReturnCode nextBatch(size_t limit, SVector* columns, size_t* len) override;

  size_t getColumnCount() const override;
  SType getColumnType(size_t idx) const override;

  bool next(SValue* row, size_t row_len) override;

protected:
  Transaction* txn_;
  String table_name_;
  Vector<eventql::TablePartitionInfo> rows_;
  size_t counter_;
};

} //csql

