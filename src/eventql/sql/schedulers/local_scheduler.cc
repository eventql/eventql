/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/schedulers/local_scheduler.h>
#include <eventql/sql/runtime/queryplan.h>

using namespace stx;

namespace csql {

ScopedPtr<ResultCursor> LocalScheduler::execute(
    QueryPlan* query_plan,
    size_t stmt_idx) {
  auto table_expr = buildExpression(
      query_plan->getTransaction(),
      query_plan->getStatement(stmt_idx));

  return table_expr->execute();
};

//ScopedPtr<TableExpression> LocalScheduler::buildExpression(
//    Transaction* ctx,
//    RefPtr<QueryTreeNode> node);
//
} // namespace csql
