/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <eventql/sql/backends/mysql/mysqltableref.h>
#include <eventql/sql/tasks/tablescan.h>

namespace csql {
namespace mysql_backend {

MySQLTableRef::MySQLTableRef(
      std::shared_ptr<MySQLConnection> conn,
      const std::string& table_name) :
      conn_(conn),
      table_name_(table_name) {
  table_columns_ = conn->describeTable(table_name_); // FIXPAUL cache me
}

std::vector<std::string> MySQLTableRef::columns() {
  std::vector<std::string> cols;

  for (const auto& col : columns_) {
    cols.emplace_back(*col);
  }

  return cols;
}

int MySQLTableRef::getComputedColumnIndex(const std::string& name) {
  for (int i = 0; i < columns_.size(); ++i) {
    // FIXPAUL case insensitive match
    if (*columns_[i] == name) {
      return i;
    }
  }

  for (int i = 0; i < table_columns_.size(); ++i) {
    // FIXPAUL case insensitive match
    if (table_columns_[i] == name) {
      columns_.emplace_back(&table_columns_[i]);
      return columns_.size() - 1;
    }
  }

  return -1;
}

std::string MySQLTableRef::getColumnName(int index) {
  if (index >= columns_.size()) {
    RAISE(kIndexError, "no such column");
  }

  return *columns_[index];
}

void MySQLTableRef::executeScan(TableScan* scan) {
  std::string mysql_query = "SELECT";

  for (int i = 0; i < columns_.size(); ++i) {
    mysql_query.append(i == 0 ? " " : ",");
    mysql_query.append("`");
    mysql_query.append(*columns_[i]); // FIXPAUL escape?
    mysql_query.append("`");
  }

  mysql_query.append(" FROM ");
  mysql_query.append(table_name_);

  conn_->executeQuery(
      mysql_query,
      [this, scan] (const std::vector<std::string>& row) -> bool {
        std::vector<SValue> row_svals;

        for (const auto& col : row) {
          row_svals.emplace_back(col);
        }

        return scan->nextRow(row_svals.data(), row_svals.size());
      });
}

}
}
