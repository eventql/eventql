/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <csql/runtime/queryplan.h>
#include <csql/runtime/queryplanbuilder.h>

namespace csql {

class ExecutionStrategy : public RefCounted {
public:

  virtual RefPtr<TableProvider> tableProvider() const = 0;

  virtual RefPtr<QueryTreeNode> rewriteQueryTree(
      RefPtr<QueryTreeNode> query) const = 0;

};

class DefaultExecutionStrategy : public ExecutionStrategy {
public:
  typedef
      Function<RefPtr<QueryTreeNode> (RefPtr<QueryTreeNode> query)>
      QueryTreeRewriteRule;

  DefaultExecutionStrategy();

  RefPtr<TableProvider> tableProvider() const override;

  void addTableProvider(RefPtr<TableProvider> provider);

  RefPtr<QueryTreeNode> rewriteQueryTree(
      RefPtr<QueryTreeNode> query) const override;

  void addQueryTreeRewriteRule(QueryTreeRewriteRule fn);

protected:
  RefPtr<TableRepository> tables_;
  Vector<QueryTreeRewriteRule> qtree_rewrite_rules_;
};

}
