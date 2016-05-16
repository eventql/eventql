  /**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#include <eventql/util/stdtypes.h>
#include <eventql/util/SHA1.h>
#include <eventql/sql/runtime/defaultruntime.h>

namespace csql {

class GroupByExpression : public TableExpression {
public:

  GroupByExpression(
      Transaction* txn,
      Vector<ValueExpression> select_expressions,
      Vector<ValueExpression> group_expressions,
      ScopedPtr<TableExpression> input);

  ~GroupByExpression();

  ScopedPtr<ResultCursor> execute() override;

  size_t getNumColumns() const override;

protected:

  bool next(SValue* row, size_t row_len);

  void freeResult();

  Transaction* txn_;
  Vector<ValueExpression> select_exprs_;
  Vector<ValueExpression> group_exprs_;
  ScopedPtr<TableExpression> input_;
  HashMap<String, Vector<VM::Instance>> groups_;
  HashMap<String, Vector<VM::Instance>>::const_iterator groups_iter_;
  ScratchMemory scratch_;
  bool freed_;
};

class PartialGroupByExpression : public TableExpression {
public:

  PartialGroupByExpression(
      Transaction* txn,
      Vector<ValueExpression> select_expressions,
      Vector<ValueExpression> group_expressions,
      ScopedPtr<TableExpression> input);

  ~PartialGroupByExpression();

  ScopedPtr<ResultCursor> execute() override;

  size_t getNumColumns() const override;

protected:

  bool next(SValue* row, size_t row_len);

  void freeResult();

  Transaction* txn_;
  Vector<ValueExpression> select_exprs_;
  Vector<ValueExpression> group_exprs_;
  ScopedPtr<TableExpression> input_;
  HashMap<String, Vector<VM::Instance>> groups_;
  HashMap<String, Vector<VM::Instance>>::const_iterator groups_iter_;
  ScratchMemory scratch_;
  bool freed_;
};

class GroupByMergeExpression : public TableExpression {
public:

  GroupByMergeExpression(
      Transaction* txn,
      Vector<ValueExpression> select_expressions,
      ScopedPtr<TableExpression> input);

  ~GroupByMergeExpression();

  ScopedPtr<ResultCursor> execute() override;

  size_t getNumColumns() const override;

protected:

  bool next(SValue* row, size_t row_len);
  void freeResult();

  Transaction* txn_;
  Vector<ValueExpression> select_exprs_;
  ScopedPtr<TableExpression> input_;
  HashMap<String, Vector<VM::Instance>> groups_;
  HashMap<String, Vector<VM::Instance>>::const_iterator groups_iter_;
  ScratchMemory scratch_;
  bool freed_;
};



}
