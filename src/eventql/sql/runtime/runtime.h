/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_SQL_RUNTIME_H
#define _FNORDMETRIC_SQL_RUNTIME_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include <eventql/util/thread/threadpool.h>
#include <eventql/sql/parser/parser.h>
#include <eventql/sql/qtree/RemoteAggregateParams.pb.h>
#include <eventql/sql/runtime/queryplan.h>
#include <eventql/sql/runtime/queryplanbuilder.h>
#include <eventql/sql/runtime/QueryBuilder.h>
#include <eventql/sql/runtime/symboltable.h>
#include <eventql/sql/runtime/ResultFormat.h>
#include <eventql/sql/runtime/ExecutionStrategy.h>
#include <eventql/sql/runtime/resultlist.h>

namespace csql {

class Runtime : public RefCounted {
public:

  static RefPtr<Runtime> getDefaultRuntime();

  // FIXPAUL: make parser configurable via parserfactory
  Runtime(
      stx::thread::ThreadPoolOptions tpool_opts,
      RefPtr<SymbolTable> symbol_table,
      RefPtr<QueryBuilder> query_builder,
      RefPtr<QueryPlanBuilder> query_plan_builder);

  ScopedPtr<Transaction> newTransaction();

  ScopedPtr<QueryPlan> buildQueryPlan(
      Transaction* ctx,
      const String& query,
      RefPtr<ExecutionStrategy> execution_strategy);

  ScopedPtr<QueryPlan> buildQueryPlan(
      Transaction* ctx,
      Vector<RefPtr<csql::QueryTreeNode>> statements,
      RefPtr<ExecutionStrategy> execution_strategy);

  SValue evaluateScalarExpression(
      Transaction* ctx,
      const String& expr,
      int argc,
      const SValue* argv);

  SValue evaluateScalarExpression(
      Transaction* ctx,
      ASTNode* expr,
      int argc,
      const SValue* argv);

  SValue evaluateScalarExpression(
      Transaction* ctx,
      RefPtr<ValueExpressionNode> expr,
      int argc,
      const SValue* argv);

  SValue evaluateScalarExpression(
      Transaction* ctx,
      const ValueExpression& expr,
      int argc,
      const SValue* argv);

  SValue evaluateConstExpression(Transaction* ctx, const String& expr);
  SValue evaluateConstExpression(Transaction* ctx, ASTNode* expr);
  SValue evaluateConstExpression(Transaction* ctx, RefPtr<ValueExpressionNode> expr);
  SValue evaluateConstExpression(Transaction* ctx, const ValueExpression& expr);

  Option<String> cacheDir() const;
  void setCacheDir(const String& cachedir);

  RefPtr<QueryBuilder> queryBuilder() const;
  RefPtr<QueryPlanBuilder> queryPlanBuilder() const;

  TaskScheduler* scheduler();
  SymbolTable* symbols();

protected:
  thread::ThreadPool tpool_;
  RefPtr<SymbolTable> symbol_table_;
  RefPtr<QueryBuilder> query_builder_;
  RefPtr<QueryPlanBuilder> query_plan_builder_;
  Option<String> cachedir_;
};

}
#endif
