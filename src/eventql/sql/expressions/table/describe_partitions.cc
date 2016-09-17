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
#include <eventql/sql/expressions/table/describe_partitions.h>
#include <eventql/sql/transaction.h>

namespace csql {

DescribePartitionsExpression::DescribePartitionsExpression(
    Transaction* txn,
    const String& table_name) :
    txn_(txn),
    table_name_(table_name),
    counter_(0) {}

ScopedPtr<ResultCursor> DescribePartitionsExpression::execute() {
  txn_->getTableProvider()->listPartitions(
      table_name_,
      [this] (const TablePartitionInfo& p_info) {
    rows_.emplace_back(p_info);
  });

  return mkScoped(
      new DefaultResultCursor(
          kNumColumns,
          std::bind(
              &DescribePartitionsExpression::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));

}

size_t DescribePartitionsExpression::getNumColumns() const {
  return kNumColumns;
}

bool DescribePartitionsExpression::next(SValue* row, size_t row_len) {
  if (counter_ < rows_.size()) {
    const auto& col = rows_[counter_];
    switch (row_len) {
      default:
      case 3: {
        std::string server_ids;
        for (size_t i = 0; i < col.server_ids.size(); ++i) {
          if (i > 0) {
            server_ids += ", ";
          }
          server_ids += col.server_ids[i];
        }
        row[2] = SValue::newString(server_ids); //Server id
      }
      case 2:
        row[1] = SValue::newString(col.keyrange_begin); //Keyrange begin
      case 1:
        row[0] = SValue::newString(col.partition_id); //Partition id
      case 0:
        break;
    }

    ++counter_;
    return true;
  } else {
    return false;
  }
}

} //csql

