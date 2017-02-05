/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/sql/qtree/SequentialScanNode.h>
#include <eventql/sql/table_expression.h>
#include <eventql/sql/runtime/ValueExpression.h>
#include <eventql/io/cstable/cstable_reader.h>

#include "eventql/eventql.h"

namespace csql {

class AbstractCSTableScan : public TableExpression {
public:

  virtual void setFilter(std::vector<bool>&& filter) = 0;

};

class CSTableScan : public AbstractCSTableScan {
public:

  CSTableScan(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<SequentialScanNode> stmt,
      const String& cstable_filename);

  CSTableScan(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<SequentialScanNode> stmt,
      RefPtr<cstable::CSTableReader> cstable,
      const String& cstable_filename = "<unknown>");

  ReturnCode execute() override;
  ReturnCode nextBatch(SVector* columns, size_t* len) override;

  size_t getColumnCount() const override;
  SType getColumnType(size_t idx) const override;

  virtual Vector<String> columnNames() const;

  size_t rowsScanned() const;

  void setFilter(std::vector<bool>&& filter) override;
  void setColumnType(String column, SType type);

protected:

  void open();
  bool next(SValue* out, size_t out_len);

  struct ColumnRef {
    ColumnRef(RefPtr<cstable::ColumnReader> r, size_t i, SType t);
    RefPtr<cstable::ColumnReader> reader;
    size_t index;
    SType type;
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

  uint64_t findMaxRepetitionLevel(RefPtr<ValueExpressionNode> expr) const;

  void fetch();

  Transaction* txn_;
  ExecutionContext* execution_context_;
  Vector<String> column_names_;
  Vector<SType> column_types_;
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
  std::vector<bool> filter_;
  bool filter_enabled_;
  size_t filter_pos_;
  bool opened_;
  uint64_t cur_select_level_;
  uint64_t cur_fetch_level_;
  bool cur_filter_pred_;
  Vector<SValue> cur_buf_;
  size_t cur_pos_;
  VMStack vm_stack_;
};

class FastCSTableScan : public AbstractCSTableScan {
public:

  static const size_t kOutputBatchSize = 1024;

  FastCSTableScan(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<SequentialScanNode> stmt,
      const String& cstable_filename);

  FastCSTableScan(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<SequentialScanNode> stmt,
      RefPtr<cstable::CSTableReader> cstable,
      const String& cstable_filename = "<unknown>");

  ~FastCSTableScan();

  ReturnCode execute() override;
  ReturnCode nextBatch(SVector* columns, size_t* len) override;

  size_t getColumnCount() const override;
  SType getColumnType(size_t idx) const override;

  void setFilter(std::vector<bool>&& filter) override;

protected:

  ReturnCode fetchColumnUInt64(size_t idx, size_t batch_size);

  Transaction* txn_;
  ExecutionContext* execution_context_;
  RefPtr<SequentialScanNode> stmt_;
  String cstable_filename_;
  RefPtr<cstable::CSTableReader> cstable_;
  std::vector<ValueExpression> select_list_;
  ValueExpression where_expr_;
  size_t records_remaining_;
  size_t records_consumed_;
  std::vector<SType> column_types_;
  std::vector<SVector> column_buffers_;
  std::vector<RefPtr<cstable::ColumnReader>> column_readers_;
  VMStack vm_stack_;
  std::vector<bool> filter_;
  bool filter_enabled_;
};

} // namespace csql
