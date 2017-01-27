/**
 * Copyright (c) 2017 DeepCortex GmbH <legal@eventql.io>
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
#include "eventql/sql/expressions/table/show_databases.h"
#include "eventql/sql/transaction.h"

namespace csql {

ShowDatabasesExpression::ShowDatabasesExpression(
    Transaction* txn) :
    txn_(txn),
    counter_(0) {}

ScopedPtr<ResultCursor> ShowDatabasesExpression::execute() {
  //auto rc = txn_->getTableProvider()->listServers(
  //    [this] (const eventql::ServerConfig& server) {
  //  rows_.emplace_back(server);
  //});

  //if (!rc.isSuccess()) {
  //  //FIXME handle error
  //}

  return mkScoped(
      new DefaultResultCursor(
          kNumColumns,
          std::bind(
              &ShowDatabasesExpression::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));

}

size_t ShowDatabasesExpression::getNumColumns() const {
  return kNumColumns;
}

bool ShowDatabasesExpression::next(SValue* row, size_t row_len) {
  if (counter_ < rows_.size()) {

    ++counter_;
    return true;
  } else {
    return false;
  }
}

} //csql

