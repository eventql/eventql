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
#include <eventql/util/stdtypes.h>
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/sql/qtree/SequentialScanNode.h>
#include <eventql/sql/expressions/table_expression.h>
#include <eventql/sql/runtime/ValueExpression.h>
#include <eventql/infra/cstable/CSTableReader.h>

using namespace stx;

namespace csql {

class CSTableScan : public TableExpression {
public:

  CSTableScan(
      Transaction* txn,
      RefPtr<SequentialScanNode> stmt,
      const String& cstable_filename);

  CSTableScan(
      Transaction* txn,
      RefPtr<SequentialScanNode> stmt,
      RefPtr<cstable::CSTableReader> cstable);

  ScopedPtr<ResultCursor> execute() override;

  virtual Vector<String> columnNames() const;
  virtual size_t numColumns() const;

  size_t rowsScanned() const;

  void setFilter(Function<bool ()> filter_fn);
  void setColumnType(String column, sql_type type);

protected:

  void open();
  bool next(SValue* out, int out_len);

  struct ColumnRef {
    ColumnRef(RefPtr<cstable::ColumnReader> r, size_t i, sql_type t);
    RefPtr<cstable::ColumnReader> reader;
    size_t index;
    sql_type type;
  };

  struct ExpressionRef {
    ExpressionRef(
        Transaction* _txn,
        size_t _rep_level,
        ValueExpression _compiled,
        ScratchMemory* scratch);

    ExpressionRef(ExpressionRef&& other);
    ~ExpressionRef();

    Transaction* txn;
    size_t rep_level;
    ValueExpression compiled;
    VM::Instance instance;
  };

  bool fetchNext(SValue* out, int out_len);
  bool fetchNextWithoutColumns(SValue* out, int out_len);

  void findColumns(
      RefPtr<ValueExpressionNode> expr,
      Set<String>* column_names) const;

  void resolveColumns(RefPtr<ValueExpressionNode> expr) const;

  uint64_t findMaxRepetitionLevel(RefPtr<ValueExpressionNode> expr) const;

  void fetch();

  Transaction* txn_;
  Vector<String> column_names_;
  ScratchMemory scratch_;
  RefPtr<SequentialScanNode> stmt_;
  String cstable_filename_;
  RefPtr<cstable::CSTableReader> cstable_;
  HashMap<String, ColumnRef> columns_;
  Vector<ExpressionRef> select_list_;
  ValueExpression where_expr_;
  size_t colindex_;
  AggregationStrategy aggr_strategy_;
  Option<SHA1Hash> cache_key_;
  size_t rows_scanned_;
  size_t num_records_;
  Function<bool ()> filter_fn_;
  bool opened_;
  uint64_t cur_select_level_;
  uint64_t cur_fetch_level_;
  bool cur_filter_pred_;
  Vector<SValue> cur_buf_;
  size_t cur_pos_;
};


} // namespace csql
