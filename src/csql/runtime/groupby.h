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
#include <stx/SHA1.h>
#include <csql/runtime/TableExpression.h>
#include <csql/runtime/defaultruntime.h>

namespace csql {

class GroupByExpression : public TableExpression {
public:

  GroupByExpression(
      const Vector<String>& column_names,
      Vector<ValueExpression> select_expressions);

  virtual void accumulate(
      HashMap<String, Vector<VM::Instance >>* groups,
      ScratchMemory* scratch,
      ExecutionContext* context) = 0;

  void execute(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn);

  void executeRemote(
      ExecutionContext* context,
      OutputStream* os);

  void getResult(
      const HashMap<String, Vector<VM::Instance >>* groups,
      Function<bool (int argc, const SValue* argv)> fn);

  void freeResult(
      HashMap<String, Vector<VM::Instance >>* groups);

  void mergeResult(
      const HashMap<String, Vector<VM::Instance >>* src,
      HashMap<String, Vector<VM::Instance >>* dst,
      ScratchMemory* scratch);

  Vector<String> columnNames() const override;

  size_t numColumns() const override;

protected:

  void encode(
      const HashMap<String, Vector<VM::Instance >>* groups,
      OutputStream* os) const;

  bool decode(
      HashMap<String, Vector<VM::Instance >>* groups,
      ScratchMemory* scratch,
      InputStream* is) const;

  Vector<String> column_names_;
  Vector<ValueExpression> select_exprs_;
};

class GroupBy : public GroupByExpression {
public:

  GroupBy(
      ScopedPtr<TableExpression> source,
      const Vector<String>& column_names,
      Vector<ValueExpression> select_expressions,
      Vector<ValueExpression> group_expressions,
      SHA1Hash qtree_fingerprint);

  void prepare(ExecutionContext* context) override;

  void accumulate(
      HashMap<String, Vector<VM::Instance >>* groups,
      ScratchMemory* scratch,
      ExecutionContext* context) override;

  Option<SHA1Hash> cacheKey() const override;

protected:

  bool nextRow(
      HashMap<String, Vector<VM::Instance >>* groups,
      ScratchMemory* scratch,
      int argc,
      const SValue* argv);

  ScopedPtr<TableExpression> source_;
  Vector<ValueExpression> group_exprs_;
  SHA1Hash qtree_fingerprint_;
};

class RemoteGroupBy : public GroupByExpression {
public:
  typedef
      Function<ScopedPtr<InputStream> (const RemoteAggregateParams& params)>
      RemoteExecuteFn;

  RemoteGroupBy(
      const Vector<String>& column_names,
      Vector<ValueExpression> select_expressions,
      const RemoteAggregateParams& params,
      RemoteExecuteFn execute_fn);

  void prepare(ExecutionContext* context) override;

  void accumulate(
      HashMap<String, Vector<VM::Instance >>* groups,
      ScratchMemory* scratch,
      ExecutionContext* context) override;

protected:
  RemoteAggregateParams params_;
  RemoteExecuteFn execute_fn_;
};

}
