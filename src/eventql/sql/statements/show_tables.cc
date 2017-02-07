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
#include <eventql/sql/statements/show_tables.h>
#include <eventql/sql/transaction.h>

namespace csql {

const std::vector<std::pair<std::string, SType>> ShowTablesExpression::kOutputColumns = {
  { "table_name", SType::STRING },
  { "description", SType::STRING }
};

ShowTablesExpression::ShowTablesExpression(
    Transaction* txn) :
    SimpleTableExpression(kOutputColumns),
    txn_(txn) {}

ReturnCode ShowTablesExpression::execute() {
  txn_->getTableProvider()->listTables([this] (const TableInfo& table) {
    Vector<SValue> row;
    row.emplace_back(SValue::newString(table.table_name));
    row.emplace_back(SValue::newString(""));
    addRow(row.data());
  });

  return ReturnCode::success();
}

}
