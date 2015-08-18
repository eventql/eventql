/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/ExecutionStrategy.h>
#include <csql/runtime/tablerepository.h>

namespace csql {

DefaultExecutionStrategy::DefaultExecutionStrategy() :
    tables_(new TableRepository()) {}

RefPtr<TableProvider> DefaultExecutionStrategy::tableProvider() const {
  return tables_.get();
}

void DefaultExecutionStrategy::addTableProvider(
    RefPtr<TableProvider> provider) {
  tables_->addProvider(provider);
}

RefPtr<QueryTreeNode> DefaultExecutionStrategy::rewriteQueryTree(
    RefPtr<QueryTreeNode> query) const {
  for (const auto& fn : qtree_rewrite_rules_) {
    query = fn(query);
  }

  return query;
}

void DefaultExecutionStrategy::addQueryTreeRewriteRule(
    QueryTreeRewriteRule fn) {
  qtree_rewrite_rules_.emplace_back(fn);
}

}
