/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/stdtypes.h>
#include <fnord/protobuf/MessageSchema.h>
#include <chartsql/Statement.h>
#include <chartsql/qtree/SelectProjectAggregateNode.h>
#include <cstable/CSTableReader.h>

using namespace fnord;

namespace csql {

class CSTableScan : public Statement {
public:

  CSTableScan(
      RefPtr<SelectProjectAggregateNode> stmt,
      cstable::CSTableReader&& cstable);

  void execute(Function<bool (int argc, const SValue* argv)> fn) override;

protected:

  struct ColumnRef {
    ColumnRef(RefPtr<cstable::ColumnReader> r);

    RefPtr<cstable::ColumnReader> reader;
    void* cur_data;
    size_t cur_size;
  };

  struct ExpressionRef {
    size_t rep_level;
  };

  void findColumns(
      RefPtr<ScalarExpressionNode> expr,
      Set<String>* column_names) const;

  uint64_t findMaxRepetitionLevel(RefPtr<ScalarExpressionNode> expr) const;

  void fetch();

  cstable::CSTableReader cstable_;
  HashMap<String, ColumnRef> columns_;
  Vector<ExpressionRef> select_list_;
};

} // namespace csql
