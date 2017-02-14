/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#pragma once
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include <eventql/util/thread/threadpool.h>
#include <eventql/sql/parser/parser.h>
#include <eventql/sql/query_plan.h>
#include <eventql/sql/runtime/queryplanbuilder.h>
#include <eventql/sql/runtime/QueryBuilder.h>
#include <eventql/sql/runtime/symboltable.h>

namespace csql {
class Scheduler;

class Runtime : public RefCounted {
public:

  static RefPtr<Runtime> getDefaultRuntime();

  // FIXPAUL: make parser configurable via parserfactory
  Runtime(
      thread::ThreadPoolOptions tpool_opts,
      RefPtr<SymbolTable> symbol_table,
      RefPtr<QueryBuilder> query_builder,
      RefPtr<QueryPlanBuilder> query_plan_builder,
      ScopedPtr<Scheduler> scheduler);

  ScopedPtr<Transaction> newTransaction();

  ScopedPtr<QueryPlan> buildQueryPlan(
      Transaction* ctx,
      const String& query);

  ScopedPtr<QueryPlan> buildQueryPlan(
      Transaction* ctx,
      Vector<RefPtr<csql::QueryTreeNode>> statements);

  Option<String> cacheDir() const;
  void setCacheDir(const String& cachedir);

  RefPtr<QueryBuilder> getCompiler() const;
  RefPtr<QueryBuilder> queryBuilder() const;
  RefPtr<QueryPlanBuilder> queryPlanBuilder() const;

  SymbolTable* symbols();

  void setScheduler(ScopedPtr<Scheduler> scheduler);
  Scheduler* getScheduler();

  QueryCache* getQueryCache() const;
  void setQueryCache(QueryCache* cache);

  SValue evaluateConstExpression(Transaction* txn, ASTNode* expr);
  SValue evaluateConstExpression(Transaction* txn, RefPtr<ValueExpressionNode> expr);

protected:
  thread::ThreadPool tpool_;
  RefPtr<SymbolTable> symbol_table_;
  RefPtr<QueryBuilder> query_builder_;
  RefPtr<QueryPlanBuilder> query_plan_builder_;
  Option<String> cachedir_;
  ScopedPtr<Scheduler> scheduler_;
  QueryCache* query_cache_;
};

} // namespace sql

