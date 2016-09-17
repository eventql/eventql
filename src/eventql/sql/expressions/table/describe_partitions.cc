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

DescribePartitionsStatement::DescribePartitionsStatement(
    Transaction* txn,
    const String& table_name) :
    txn_(txn),
    table_name_(table_name),
    counter_(0) {}

ScopedPtr<ResultCursor> DescribePartitionsStatement::execute() {
  txn_->getTableProvider()->listPartitions(
      table_name_,
      [this] (const MetadataFile::PartitionMapEntry& partition) {
    std::vector<SValue> row;

    std::string server_id_str;
    for (const auto& s : partition.servers) {
      server_id_str += s.server_id;
    }
    row.emplace_back(server_id_str); //FIXME
    row.emplace_back(partition.partition_id);
    switch (metadata_file.getKeyspaceType()) {
      case KEYSPACE_UINT64: {
        uint64_t keyrange_uint = -1;
        memcpy((char*) &keyrange_uint, e.begin.data(), sizeof(e.begin));
        row.emplace_back(StringUtil::format(
            "$0 [$1]",
            UnixTime(keyrange_uint),
            keyrange_uint_);
        break;
      }
      case KEYSPACE_STRING: {
        row.emplace_back(e.begin);
        break;
      }
    }

    buf_.emplace_back(row);
  });

  return mkScoped(
      new DefaultResultCursor(
          kNumColumns,
          std::bind(
              &DescribePartitionsStatement::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));

}

size_t DescribePartitionsStatement::getNumColumns() const {
  return kNumColumns;
}

bool DescribePartitionsStatement::next(SValue* row, size_t row_len) {
  return false;
  //if (counter_ < rows_.size()) {
  //  const auto& col = rows_[counter_];
  //  switch (row_len) {
  //    default:
  //    case 4:
  //      row[3] = SValue::newString(col.server_id); //Server id
  //    case 3:
  //      row[2] = SValue::newString(col.keyrange_end); //Keyrange end
  //    case 2:
  //      row[1] = SValue::newString(col.keyrange_begin); //Keyrange begin
  //    case 1:
  //      row[0] = SValue::newString(col.partition_id); //Partition id
  //    case 0:
  //      break;
  //  }

  //  ++counter_;
  //  return true;
  //} else {
  //  return false;
  //}
}

} //csql

