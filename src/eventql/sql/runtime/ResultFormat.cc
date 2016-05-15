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
#include <eventql/sql/runtime/ResultFormat.h>

namespace csql {

void CallbackResultHandler::onRow(
    Function<void (size_t stmt_id, int argc, const SValue* argv)> fn) {
  on_row_ = fn;
}

void CallbackResultHandler::formatResults(
    ScopedPtr<QueryPlan> query,
    ExecutionContext* context) {
  RAISE(kNotImplementedError);
  //for (int i = 0; i < query->numStatements(); ++i) {
  //  auto stmt = query->getStatement(i);
  //  auto select = dynamic_cast<TableExpression*>(stmt);
  //  if (!select) {
  //    RAISE(
  //        kRuntimeError,
  //        "can't execute non select statement in CallbackResultHandler");
  //  }

  //  select->execute(
  //      context,
  //      [this, i] (int argc, const csql::SValue* argv) -> bool {
  //    on_row_(i, argc, argv);
  //    return true;
  //  });
  //}
}

}
