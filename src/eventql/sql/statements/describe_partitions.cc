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
#include <assert.h>
#include <eventql/sql/statements/describe_partitions.h>
#include <eventql/sql/transaction.h>

namespace csql {

const std::vector<std::pair<std::string, SType>> DescribePartitionsExpression::kOutputColumns = {
  { "partition_id", SType::STRING },
  { "servers", SType::STRING },
  { "keyrange_begin", SType::STRING },
  { "keyrange_end", SType::STRING },
  { "extra info", SType::STRING }
};

DescribePartitionsExpression::DescribePartitionsExpression(
    Transaction* txn,
    const String& table_name) :
    SimpleTableExpression(kOutputColumns),
    txn_(txn),
    table_name_(table_name) {}

ReturnCode DescribePartitionsExpression::execute() {
  txn_->getTableProvider()->listPartitions(
      table_name_,
      [this] (const eventql::TablePartitionInfo& p_info) {
    std::vector<SValue> row;
    row.emplace_back(SValue::newString(p_info.partition_id)); //Partition id
    row.emplace_back(SValue::newString(
        StringUtil::join(p_info.server_ids, ","))); //Server id
    row.emplace_back(SValue::newString(p_info.keyrange_begin)); //Keyrange begin
    row.emplace_back(SValue::newString(p_info.keyrange_end)); //Keyrange end
    row.emplace_back(SValue::newString(p_info.extra_info)); //Extra info
    addRow(row.data());
  });

  return ReturnCode::success();
}

} // namespace csql

