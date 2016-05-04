/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/runtime.h>
#include <csql/tasks/groupby.h>
#include <csql/qtree/QueryTreeUtil.h>
#include <csql/schedulers/local_scheduler.h>
#include <csql/defaults.h>

namespace csql {

ScopedPtr<Transaction> Runtime::newTransaction() {
  return mkScoped(new Transaction(this));
}

RefPtr<Runtime> Runtime::getDefaultRuntime() {
  auto symbols = mkRef(new SymbolTable());
  installDefaultSymbols(symbols.get());

  return new Runtime(
      stx::thread::ThreadPoolOptions{},
      symbols,
      new QueryBuilder(
          new ValueExpressionBuilder(symbols.get())),
      new QueryPlanBuilder(
          QueryPlanBuilderOptions{},
          symbols.get()));
}

Runtime::Runtime(
    stx::thread::ThreadPoolOptions tpool_opts,
    RefPtr<SymbolTable> symbol_table,
    RefPtr<QueryBuilder> query_builder,
    RefPtr<QueryPlanBuilder> query_plan_builder) :
    tpool_(tpool_opts),
    symbol_table_(symbol_table),
    query_builder_(query_builder),
    query_plan_builder_(query_plan_builder) {}

ScopedPtr<QueryPlan> Runtime::buildQueryPlan(
    Transaction* txn,
    const String& query,
    RefPtr<ExecutionStrategy> execution_strategy) {
  /* parse query */
  csql::Parser parser;
  parser.parse(query.data(), query.size());

  /* build query plan */
  auto statements = query_plan_builder_->build(
      txn,
      parser.getStatements(),
      execution_strategy->tableProvider());

  return buildQueryPlan(
      txn,
      statements,
      execution_strategy);
}

ScopedPtr<QueryPlan> Runtime::buildQueryPlan(
    Transaction* txn,
    Vector<RefPtr<QueryTreeNode>> statements,
    RefPtr<ExecutionStrategy> execution_strategy) {
  for (auto& stmt : statements) {
    stmt = execution_strategy->rewriteQueryTree(stmt);
  }

  auto qplan = mkScoped(new QueryPlan(txn, statements));
  qplan->setScheduler(LocalScheduler::getFactory());
  return std::move(qplan);
}

SValue Runtime::evaluateScalarExpression(
    Transaction* txn,
    ASTNode* expr,
    int argc,
    const SValue* argv) {
  auto val_expr = query_plan_builder_->buildValueExpression(txn, expr);
  auto compiled = query_builder_->buildValueExpression(txn, val_expr);

  SValue out;
  VM::evaluate(txn, compiled.program(), argc, argv, &out);
  return out;
}

SValue Runtime::evaluateScalarExpression(
    Transaction* txn,
    const String& expr,
    int argc,
    const SValue* argv) {
  csql::Parser parser;
  parser.parseValueExpression(expr.data(), expr.size());

  auto stmts = parser.getStatements();
  if (stmts.size() != 1) {
    RAISE(
        kParseError,
        "static expression must consist of exactly one statement");
  }

  auto val_expr = query_plan_builder_->buildValueExpression(txn, stmts[0]);
  auto compiled = query_builder_->buildValueExpression(txn, val_expr);

  SValue out;
  VM::evaluate(txn, compiled.program(), argc, argv, &out);
  return out;
}

SValue Runtime::evaluateScalarExpression(
    Transaction* txn,
    RefPtr<ValueExpressionNode> expr,
    int argc,
    const SValue* argv) {
  auto compiled = query_builder_->buildValueExpression(txn, expr);

  SValue out;
  VM::evaluate(txn, compiled.program(), argc, argv, &out);
  return out;
}

SValue Runtime::evaluateScalarExpression(
    Transaction* txn,
    const ValueExpression& expr,
    int argc,
    const SValue* argv) {
  SValue out;
  VM::evaluate(txn, expr.program(), argc, argv, &out);
  return out;
}

SValue Runtime::evaluateConstExpression(Transaction* txn, ASTNode* expr) {
  auto val_expr = query_plan_builder_->buildValueExpression(txn,expr);
  auto compiled = query_builder_->buildValueExpression(txn, val_expr);

  SValue out;
  VM::evaluate(txn, compiled.program(), 0, nullptr, &out);
  return out;
}

SValue Runtime::evaluateConstExpression(Transaction* txn, const String& expr) {
  csql::Parser parser;
  parser.parseValueExpression(expr.data(), expr.size());

  auto stmts = parser.getStatements();
  if (stmts.size() != 1) {
    RAISE(
        kParseError,
        "static expression must consist of exactly one statement");
  }

  auto val_expr = query_plan_builder_->buildValueExpression(txn, stmts[0]);
  auto compiled = query_builder_->buildValueExpression(txn, val_expr);

  SValue out;
  VM::evaluate(txn, compiled.program(), 0, nullptr, &out);
  return out;
}

SValue Runtime::evaluateConstExpression(
    Transaction* txn,
    RefPtr<ValueExpressionNode> expr) {
  auto compiled = query_builder_->buildValueExpression(txn, expr);

  SValue out;
  VM::evaluate(txn, compiled.program(), 0, nullptr, &out);
  return out;
}

SValue Runtime::evaluateConstExpression(
    Transaction* txn,
    const ValueExpression& expr) {
  SValue out;
  VM::evaluate(txn, expr.program(), 0, nullptr, &out);
  return out;
}

Option<String> Runtime::cacheDir() const {
  return cachedir_;
}

void Runtime::setCacheDir(const String& cachedir) {
  cachedir_ = Some(cachedir);
}

RefPtr<QueryBuilder> Runtime::queryBuilder() const {
  return query_builder_;
}

RefPtr<QueryPlanBuilder> Runtime::queryPlanBuilder() const {
  return query_plan_builder_;
}

TaskScheduler* Runtime::scheduler() {
  return &tpool_;
}

SymbolTable* Runtime::symbols() {
  return symbol_table_.get();
}


}
