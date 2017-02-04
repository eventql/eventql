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
#include <eventql/sql/runtime/runtime.h>
#include <eventql/sql/qtree/QueryTreeUtil.h>
#include <eventql/sql/scheduler.h>
#include <eventql/sql/defaults.h>

namespace csql {

ScopedPtr<Transaction> Runtime::newTransaction() {
  return mkScoped(new Transaction(this));
}

RefPtr<Runtime> Runtime::getDefaultRuntime() {
  auto symbols = mkRef(new SymbolTable());
  installDefaultSymbols(symbols.get());

  return new Runtime(
      thread::ThreadPoolOptions{},
      symbols,
      new QueryBuilder(
          new ValueExpressionBuilder(symbols.get())),
      new QueryPlanBuilder(
          QueryPlanBuilderOptions{},
          symbols.get()),
      mkScoped(new DefaultScheduler()));
}

Runtime::Runtime(
    thread::ThreadPoolOptions tpool_opts,
    RefPtr<SymbolTable> symbol_table,
    RefPtr<QueryBuilder> query_builder,
    RefPtr<QueryPlanBuilder> query_plan_builder,
    ScopedPtr<Scheduler> scheduler) :
    tpool_(tpool_opts),
    symbol_table_(symbol_table),
    query_builder_(query_builder),
    query_plan_builder_(query_plan_builder),
    scheduler_(std::move(scheduler)),
    query_cache_(nullptr) {}

ScopedPtr<QueryPlan> Runtime::buildQueryPlan(
    Transaction* txn,
    const String& query) {
  /* parse query */
  csql::Parser parser;
  parser.parse(query.data(), query.size());

  /* build query plan */
  auto statements = query_plan_builder_->build(
      txn,
      parser.getStatements(),
      txn->getTableProvider());

  return buildQueryPlan(txn, statements);
}

ScopedPtr<QueryPlan> Runtime::buildQueryPlan(
    Transaction* txn,
    Vector<RefPtr<QueryTreeNode>> statements) {
  auto qplan = mkScoped(new QueryPlan(txn, statements));
  return std::move(qplan);
}

Option<String> Runtime::cacheDir() const {
  return cachedir_;
}

void Runtime::setCacheDir(const String& cachedir) {
  cachedir_ = Some(cachedir);
}

RefPtr<QueryBuilder> Runtime::getCompiler() const {
  return query_builder_;
}

RefPtr<QueryBuilder> Runtime::queryBuilder() const {
  return query_builder_;
}

RefPtr<QueryPlanBuilder> Runtime::queryPlanBuilder() const {
  return query_plan_builder_;
}

SymbolTable* Runtime::symbols() {
  return symbol_table_.get();
}

void Runtime::setScheduler(ScopedPtr<Scheduler> scheduler) {
  scheduler_ = std::move(scheduler);
}

Scheduler* Runtime::getScheduler() {
  return scheduler_.get();
}

QueryCache* Runtime::getQueryCache() const {
  return query_cache_;
}

void Runtime::setQueryCache(QueryCache* cache) {
  query_cache_ = cache;
}

}

