/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/ResultFormat.h>

namespace csql {

void CallbackResultHandler::onRow(
    Function<void (size_t stmt_id, int argc, const SValue* argv)> fn) {
  on_row_ = fn;
}

void CallbackResultHandler::formatResults(
    RefPtr<QueryPlan> query,
    ExecutionContext* context) {
  for (int i = 0; i < query->numStatements(); ++i) {
    auto stmt = query->getStatement(i);
    auto select = dynamic_cast<TableExpression*>(stmt);
    if (!select) {
      RAISE(
          kRuntimeError,
          "can't execute non select statement in CallbackResultHandler");
    }

    select->execute(
        context,
        [this, i] (int argc, const csql::SValue* argv) -> bool {
      on_row_(i, argc, argv);
      return true;
    });
  }
}

}
