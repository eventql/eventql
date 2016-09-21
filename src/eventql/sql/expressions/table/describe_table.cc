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
#include <eventql/sql/expressions/table/describe_table.h>
#include <eventql/sql/transaction.h>

namespace csql {

DescribeTableStatement::DescribeTableStatement(
    Transaction* txn,
    const String& table_name) :
    txn_(txn),
    table_name_(table_name),
    counter_(0) {}

ScopedPtr<ResultCursor> DescribeTableStatement::execute() {
  auto table_info = txn_->getTableProvider()->describe(table_name_);
  if (table_info.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", table_name_);
  }

  rows_ = table_info.get().columns;
  return mkScoped(
      new DefaultResultCursor(
          kNumColumns,
          std::bind(
              &DescribeTableStatement::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}

size_t DescribeTableStatement::getNumColumns() const {
  return kNumColumns;
}

bool DescribeTableStatement::next(SValue* row, size_t row_len) {
  if (counter_ < rows_.size()) {
    const auto& col = rows_[counter_];
    switch (row_len) {
      default:
      case 6:
        row[5] = SValue::newString(col.encoding); //Encoding
      case 5:
        row[4] = SValue::newNull(); //Description
      case 4:
        row[3] = col.is_primary_key ? SValue::newString("YES") : SValue::newString("NO"); //Primary Key
      case 3:
        row[2] = col.is_nullable ? SValue::newString("YES") : SValue::newString("NO"); //Null
      case 2:
        row[1] = SValue::newString(col.type); //Type
      case 1:
        row[0] = SValue::newString(col.column_name); //Field
      case 0:
        break;
    }

    ++counter_;
    return true;
  } else {
    return false;
  }
}

}
