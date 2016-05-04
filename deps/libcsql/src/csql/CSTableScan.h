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
#include <stx/stdtypes.h>
#include <stx/protobuf/MessageSchema.h>
#include <csql/qtree/SequentialScanNode.h>
#include <csql/runtime/compiler.h>
#include <csql/runtime/defaultruntime.h>
#include <csql/runtime/TableExpression.h>
#include <csql/runtime/ValueExpression.h>
#include <cstable/CSTableReader.h>

using namespace stx;

namespace csql {

class CSTableScan : public Task {
public:

  CSTableScan(
      Transaction* txn,
      RefPtr<SequentialScanNode> stmt,
      const String& cstable_filename,
      QueryBuilder* runtime,
      RowSinkFn output);

  CSTableScan(
      Transaction* txn,
      RefPtr<SequentialScanNode> stmt,
      RefPtr<cstable::CSTableReader> cstable,
      QueryBuilder* runtime,
      RowSinkFn output);

  virtual Vector<String> columnNames() const;
  virtual size_t numColumns() const;

  void onInputsReady() override;

  void open();

  Option<SHA1Hash> cacheKey() const override;
  void setCacheKey(const SHA1Hash& key);

  size_t rowsScanned() const;

  void setFilter(Function<bool ()> filter_fn);
  void setColumnType(String column, sql_type type);

protected:

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

  void scan();
  void scanWithoutColumns();

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
  QueryBuilder* runtime_;
  RowSinkFn output_;
  HashMap<String, ColumnRef> columns_;
  Vector<ExpressionRef> select_list_;
  ValueExpression where_expr_;
  size_t colindex_;
  AggregationStrategy aggr_strategy_;
  Option<SHA1Hash> cache_key_;
  size_t rows_scanned_;
  Function<bool ()> filter_fn_;
  bool opened_;
};


} // namespace csql
