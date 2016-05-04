/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <stx/SHA1.h>
#include <csql/tasks/Task.h>
#include <csql/runtime/RowSink.h>
using namespace stx;

namespace csql {
class Transaction;

class TaskFactory : public RefCounted {
public:

  virtual RefPtr<Task> build(
      Transaction* txn,
      RowSinkFn output) const = 0;

};

using TableExpressionFactoryRef = RefPtr<TaskFactory>;

class SimpleTableExpressionFactory : public TaskFactory {
public:
  typedef
      Function<RefPtr<Task> (
          Transaction* txn,
          RowSinkFn output)> FactoryFn;

  SimpleTableExpressionFactory(FactoryFn factory_fn);

  RefPtr<Task> build(
      Transaction* txn,
      RowSinkFn output) const override;

protected:
  FactoryFn factory_fn_;
};

} // namespace csql
