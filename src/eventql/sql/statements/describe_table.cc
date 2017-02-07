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
#include <eventql/sql/statements/describe_table.h>
#include <eventql/sql/transaction.h>

namespace csql {

const std::vector<std::pair<std::string, SType>> DescribeTableStatement::kOutputColumns = {
  { "column_name", SType::STRING },
  { "type", SType::STRING },
  { "nullable", SType::STRING },
  { "primary_key", SType::STRING },
  { "description", SType::STRING },
  { "encoding", SType::STRING }
};

DescribeTableStatement::DescribeTableStatement(
    Transaction* txn,
    const String& table_name) :
    SimpleTableExpression(kOutputColumns),
    txn_(txn),
    table_name_(table_name) {}

ReturnCode DescribeTableStatement::execute() {
  auto table_info = txn_->getTableProvider()->describe(table_name_);
  if (table_info.isEmpty()) {
    return ReturnCode::errorf("EARG", "table not found: $0", table_name_);
  }

  for (const auto& col : table_info.get().columns) {
    std::vector<SValue> row;
    row.emplace_back(SValue::newString(col.column_name)); //Field
    row.emplace_back(SValue::newString(sql_typename(col.type))); //Type
    row.emplace_back(col.is_nullable ? SValue::newString("YES") : SValue::newString("NO")); //Null
    row.emplace_back(col.is_primary_key ? SValue::newString("YES") : SValue::newString("NO")); //Primary Key
    row.emplace_back(SValue::newString("")); //Description
    row.back().setTag(STAG_NULL);
    row.emplace_back(SValue::newString(col.encoding)); //Encoding
    addRow(row.data());
  }

  return ReturnCode::success();
}

} // namespace csql

