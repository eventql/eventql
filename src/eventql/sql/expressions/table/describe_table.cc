/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/expressions/table/describe_table.h>
#include <eventql/sql/Transaction.h>

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

  buf_ = table_info.get().columns;
  return mkScoped(
      new DefaultResultCursor(
          kNumColumns,
          std::bind(
              &DescribeTableStatement::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}

bool DescribeTableStatement::next(SValue* row, size_t row_len) {
  if (counter_ < buf_.size()) {
    const auto& col = buf_[counter_];
    
    switch (row_len) {
      default:
      case 4:
        row[3] = SValue::newNull(); //Description
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
