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
#include <fnord/stdtypes.h>
#include <fnord/autoref.h>
#include <chartsql/runtime/tablerepository.h>
#include <chartsql/runtime/charts/drawstatement.h>

namespace csql {
class Runtime;

class QueryPlan : public RefCounted  {
public:

  QueryPlan(
      Vector<RefPtr<QueryTreeNode>> statements,
      RefPtr<TableProvider> tables,
      QueryBuilder* runtime);

  size_t numStatements() const;

  ScopedPtr<Statement> buildStatement(size_t stmt_idx) const;

protected:
  Vector<RefPtr<QueryTreeNode>> statements_;
  RefPtr<TableProvider> tables_;
  QueryBuilder* runtime_;
};

class ExecutionPlan : public RefCounted {
public:

  virtual ~ExecutionPlan() {}
  virtual void execute(Function<bool (int argc, const SValue* argv)> fn) = 0;

};

}
