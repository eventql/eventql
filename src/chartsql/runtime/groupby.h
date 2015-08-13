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
#include <chartsql/runtime/TableExpression.h>
#include <chartsql/runtime/defaultruntime.h>

namespace csql {

class GroupByExpression : public TableExpression {
public:

  virtual void accumulate(
      HashMap<String, Vector<ValueExpression::Instance>>* groups,
      ScratchMemory* scratch,
      ExecutionContext* context) = 0;

  virtual void getResult(
      const HashMap<String, Vector<ValueExpression::Instance>>* groups,
      Function<bool (int argc, const SValue* argv)> fn) = 0;

  virtual void freeResult(
      HashMap<String, Vector<ValueExpression::Instance>>* groups) = 0;

  virtual void mergeResult(
      const HashMap<String, Vector<ValueExpression::Instance>>* src,
      HashMap<String, Vector<ValueExpression::Instance>>* dst,
      ScratchMemory* scratch) = 0;

};

class GroupBy : public GroupByExpression {
public:

  GroupBy(
      ScopedPtr<TableExpression> source,
      const Vector<String>& column_names,
      Vector<ScopedPtr<ValueExpression>> select_expressions,
      Vector<ScopedPtr<ValueExpression>> group_expressions,
      SHA1Hash qtree_fingerprint);

  void execute(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn) override;

  void accumulate(
      HashMap<String, Vector<ValueExpression::Instance>>* groups,
      ScratchMemory* scratch,
      ExecutionContext* context) override;

  void getResult(
      const HashMap<String, Vector<ValueExpression::Instance>>* groups,
      Function<bool (int argc, const SValue* argv)> fn) override;

  void freeResult(
      HashMap<String, Vector<ValueExpression::Instance>>* groups) override;

  void mergeResult(
      const HashMap<String, Vector<ValueExpression::Instance>>* src,
      HashMap<String, Vector<ValueExpression::Instance>>* dst,
      ScratchMemory* scratch) override;

  Vector<String> columnNames() const override;

  size_t numColumns() const override;

  Option<SHA1Hash> cacheKey() const override;

protected:

  bool nextRow(
      HashMap<String, Vector<ValueExpression::Instance>>* groups,
      ScratchMemory* scratch,
      int argc,
      const SValue* argv);

  ScopedPtr<TableExpression> source_;
  Vector<String> column_names_;
  Vector<ScopedPtr<ValueExpression>> select_exprs_;
  Vector<ScopedPtr<ValueExpression>> group_exprs_;
  SHA1Hash qtree_fingerprint_;
};

}
