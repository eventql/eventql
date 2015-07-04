/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/stdtypes.h>
#include <chartsql/runtime/TableExpression.h>

namespace csql {

class LimitClause : public TableExpression {
public:

  LimitClause(int limit, int offset, ScopedPtr<TableExpression> child);

  void execute(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn) override;

  Vector<String> columnNames() const override;

  size_t numColumns() const override;
/*
  void execute() override {
    child_->execute();
  }

  size_t getNumCols() const override {
    return child_->getNumCols();
  }

  bool nextRow(SValue* row, int row_len) override {
    if (counter_++ < offset_) {
      return true;
    }

    if (counter_ > (offset_ + limit_)) {
      return false;
    }

    emitRow(row, row_len);
    return true;
  }

  const std::vector<std::string>& getColumns() const override {
    return child_->getColumns();
  }
*/

protected:
  size_t limit_;
  size_t offset_;
  ScopedPtr<TableExpression> child_;
};

}
