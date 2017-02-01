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
#include <assert.h>
#include <iostream>
#include "eventql/eventql.h"
#include <eventql/sql/CSTableScan.h>
#include <eventql/sql/qtree/ColumnReferenceNode.h>
#include <eventql/sql/runtime/defaultruntime.h>
#include <eventql/sql/runtime/compiler.h>
#include <eventql/util/ieee754.h>
#include <eventql/util/logging.h>

namespace csql {

CSTableScan::CSTableScan(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<SequentialScanNode> stmt,
    const String& cstable_filename) :
    txn_(txn),
    execution_context_(execution_context),
    stmt_(stmt->deepCopyAs<SequentialScanNode>()),
    cstable_filename_(cstable_filename),
    colindex_(0),
    aggr_strategy_(stmt_->aggregationStrategy()),
    rows_scanned_(0),
    num_records_(0),
    opened_(false),
    cur_select_level_(0),
    cur_fetch_level_(0),
    cur_filter_pred_(true),
    cur_pos_(0) {
  column_names_ = stmt_->selectedColumns();
  for (const auto& c : column_names_) {
    column_types_.emplace_back(stmt_->getComputedColumnInfo(c).second);
  }
  execution_context_->incrementNumTasks();
}

CSTableScan::CSTableScan(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<SequentialScanNode> stmt,
    RefPtr<cstable::CSTableReader> cstable,
    const String& cstable_filename /* = "<unknown>" */) :
    txn_(txn),
    execution_context_(execution_context),
    stmt_(stmt->deepCopyAs<SequentialScanNode>()),
    cstable_(cstable),
    cstable_filename_(cstable_filename),
    colindex_(0),
    aggr_strategy_(stmt_->aggregationStrategy()),
    rows_scanned_(0),
    num_records_(0),
    opened_(false),
    cur_select_level_(0),
    cur_fetch_level_(0),
    cur_filter_pred_(true),
    cur_pos_(0) {
  column_names_ = stmt_->selectedColumns();
  for (const auto& c : column_names_) {
    column_types_.emplace_back(stmt_->getComputedColumnInfo(c).second);
  }
  execution_context_->incrementNumTasks();
}

ReturnCode CSTableScan::execute() {
  execution_context_->incrementNumTasksRunning();
  if (!opened_) {
    open();
  }

  cur_buf_.resize(colindex_);
  num_records_ = cstable_->numRecords();

  return ReturnCode::success();
}

ReturnCode CSTableScan::nextBatch(
    size_t limit,
    SVector* columns,
    size_t* nrecords) {
  return ReturnCode::error("ERUNTIME", "CSTableScan::nextBatch not yet implemented");
}

void CSTableScan::open() {
  auto runtime = txn_->getCompiler();
  opened_ = true;

  if (cstable_.get() == nullptr) {
    cstable_ = cstable::CSTableReader::openFile(cstable_filename_);
  }

  for (const auto& col : column_names_) {
    if (!cstable_->hasColumn(col)) {
      continue;
    }

    auto reader = cstable_->getColumnReader(col);

    SType type;
    switch (reader->type()) {
      case cstable::ColumnType::STRING:
        type = SType::STRING;
        break;
      case cstable::ColumnType::SIGNED_INT:
        type = SType::INT64;
        break;
      case cstable::ColumnType::UNSIGNED_INT:
        type = SType::UINT64;
        break;
      case cstable::ColumnType::FLOAT:
        type = SType::FLOAT64;
        break;
      case cstable::ColumnType::BOOLEAN:
        type = SType::BOOL;
        break;
      case cstable::ColumnType::DATETIME:
        type = SType::TIMESTAMP64;
        break;
      case cstable::ColumnType::SUBRECORD:
        RAISE(kIllegalStateError);
    }

    columns_.emplace(col, ColumnRef(reader, colindex_++, type));
  }

  for (const auto& slnode : stmt_->selectList()) {
    select_list_.emplace_back(
        txn_,
        findMaxRepetitionLevel(slnode->expression()),
        runtime->buildValueExpression(txn_, slnode->expression()),
        &scratch_);
  }

  auto where_expr = stmt_->whereExpression();
  if (!where_expr.isEmpty()) {
    where_expr_ = runtime->buildValueExpression(txn_, where_expr.get());
  }
}

bool CSTableScan::next(SValue* out, size_t out_len) {
  try {
    if (columns_.empty()) {
      return fetchNextWithoutColumns(out, out_len);
    } else {
      return fetchNext(out, out_len);
    }
  } catch (const std::exception& e) {
    RAISEF(
        kRuntimeError,
        "error while scanning table $0: $1",
        cstable_filename_,
        e.what());
  }
}

bool CSTableScan::fetchNext(SValue* out, int out_len) {
  if (cur_pos_ >= num_records_) {
    return false;
  }

  while (cur_pos_ < num_records_) {
    if (++rows_scanned_ % 8192 == 0) {
      auto rc = txn_->triggerHeartbeat();
      if (!rc.isSuccess()) {
        RAISE(kRuntimeError, rc.getMessage());
      }
    }

    uint64_t next_level = 0;

    if (cur_fetch_level_ == 0) {
      if (cur_pos_ < num_records_ && filter_fn_) {
        cur_filter_pred_ = filter_fn_();
      }
    }

    for (auto& col : columns_) {
      auto nextr = col.second.reader->nextRepetitionLevel();

      if (nextr >= cur_fetch_level_) {
        auto& reader = col.second.reader;

        uint64_t r;
        uint64_t d;

        switch (col.second.reader->type()) {

          case cstable::ColumnType::STRING: {
            String v;
            reader->readString(&r, &d, &v);

            if (d < reader->maxDefinitionLevel()) {
              cur_buf_[col.second.index] = SValue();
            } else {
              switch (col.second.type) {
                case SType::NIL:
                  cur_buf_[col.second.index] = SValue::newNull();
                  break;
                case SType::STRING:
                  cur_buf_[col.second.index] = SValue::newString(v);
                  break;
                default:
                  assert(false); // type error
              }
            }

            break;
          }

          case cstable::ColumnType::UNSIGNED_INT: {
            uint64_t v = 0;
            reader->readUnsignedInt(&r, &d, &v);

            if (d < reader->maxDefinitionLevel()) {
              cur_buf_[col.second.index] = SValue();
            } else {
              switch (col.second.type) {
                case SType::NIL:
                  cur_buf_[col.second.index] = SValue::newNull();
                  break;
                case SType::STRING:
                  cur_buf_[col.second.index] = SValue::newInteger(v).toString();
                  break;
                case SType::FLOAT64:
                  cur_buf_[col.second.index] = SValue::newFloat(v);
                  break;
                case SType::INT64:
                  cur_buf_[col.second.index] = SValue::newInt64(v);
                  break;
                case SType::UINT64:
                  cur_buf_[col.second.index] = SValue::newUInt64(v);
                  break;
                case SType::BOOL:
                  cur_buf_[col.second.index] = SValue::newBool(v);
                  break;
                case SType::TIMESTAMP64:
                  cur_buf_[col.second.index] = SValue::newTimestamp(v);
                  break;
              }
            }

            break;
          }

          case cstable::ColumnType::SIGNED_INT: {
            int64_t v = 0;
            reader->readSignedInt(&r, &d, &v);

            if (d < reader->maxDefinitionLevel()) {
              cur_buf_[col.second.index] = SValue();
            } else {
              switch (col.second.type) {
                case SType::NIL:
                  cur_buf_[col.second.index] = SValue::newNull();
                  break;
                case SType::STRING:
                  cur_buf_[col.second.index] = SValue::newInteger(v).toString();
                  break;
                case SType::FLOAT64:
                  cur_buf_[col.second.index] = SValue::newFloat(v);
                  break;
                case SType::INT64:
                  cur_buf_[col.second.index] = SValue::newInt64(v);
                  break;
                case SType::UINT64:
                  cur_buf_[col.second.index] = SValue::newUInt64(v);
                  break;
                case SType::BOOL:
                  cur_buf_[col.second.index] = SValue::newBool(v);
                  break;
                case SType::TIMESTAMP64:
                  cur_buf_[col.second.index] = SValue::newTimestamp(v);
                  break;
              }
            }

            break;
          }

          case cstable::ColumnType::BOOLEAN: {
            bool v = 0;
            reader->readBoolean(&r, &d, &v);

            if (d < reader->maxDefinitionLevel()) {
              cur_buf_[col.second.index] = SValue(SValue::BoolType(false));
            } else {
              switch (col.second.type) {
                case SType::NIL:
                  cur_buf_[col.second.index] = SValue::newNull();
                  break;
                case SType::STRING:
                  cur_buf_[col.second.index] = SValue::newBool(v).toString();
                  break;
                case SType::FLOAT64:
                  cur_buf_[col.second.index] = SValue::newFloat(v);
                  break;
                case SType::INT64:
                  cur_buf_[col.second.index] = SValue::newInt64(v);
                  break;
                case SType::UINT64:
                  cur_buf_[col.second.index] = SValue::newUInt64(v);
                  break;
                case SType::BOOL:
                  cur_buf_[col.second.index] = SValue::newBool(v);
                  break;
                case SType::TIMESTAMP64:
                  cur_buf_[col.second.index] = SValue::newTimestamp(v);
                  break;
              }
            }

            break;
          }

          case cstable::ColumnType::FLOAT: {
            double v = 0;
            reader->readFloat(&r, &d, &v);

            if (d < reader->maxDefinitionLevel()) {
              cur_buf_[col.second.index] = SValue();
            } else {
              switch (col.second.type) {
                case SType::NIL:
                  cur_buf_[col.second.index] = SValue::newNull();
                  break;
                case SType::STRING:
                  cur_buf_[col.second.index] = SValue::newFloat(v).toString();
                  break;
                case SType::FLOAT64:
                  cur_buf_[col.second.index] = SValue::newFloat(v);
                  break;
                case SType::INT64:
                  cur_buf_[col.second.index] = SValue::newInt64(v);
                  break;
                case SType::UINT64:
                  cur_buf_[col.second.index] = SValue::newUInt64(v);
                  break;
                case SType::BOOL:
                  cur_buf_[col.second.index] = SValue::newBool(v);
                  break;
                case SType::TIMESTAMP64:
                  cur_buf_[col.second.index] = SValue::newTimestamp(v);
                  break;
              }
            }

            break;
          }

          case cstable::ColumnType::DATETIME: {
            UnixTime v;
            reader->readDateTime(&r, &d, &v);

            if (d < reader->maxDefinitionLevel()) {
              cur_buf_[col.second.index] = SValue();
            } else {
              switch (col.second.type) {
                case SType::NIL:
                  cur_buf_[col.second.index] = SValue::newNull();
                  break;
                case SType::STRING:
                  cur_buf_[col.second.index] = SValue::newTimestamp(v).toString();
                  break;
                case SType::FLOAT64:
                  cur_buf_[col.second.index] = SValue::newTimestamp(v).toFloat();
                  break;
                case SType::INT64:
                  cur_buf_[col.second.index] = SValue::newTimestamp(v).toInteger();
                  break;
                case SType::UINT64:
                  cur_buf_[col.second.index] = SValue::newTimestamp(v).toInteger();
                  break;
                case SType::TIMESTAMP64:
                  cur_buf_[col.second.index] = SValue::newTimestamp(v);
                  break;
                default:
                  RAISE(kIllegalStateError);
              }
            }

            break;
          }

          case cstable::ColumnType::SUBRECORD:
            RAISE(kIllegalStateError);

        }
      }

      next_level = std::max(
          next_level,
          col.second.reader->nextRepetitionLevel());
    }

    cur_fetch_level_ = next_level;
    if (cur_fetch_level_ == 0) {
      ++cur_pos_;
    }

    bool row_ready = false;
    bool where_pred = cur_filter_pred_;
    if (where_pred && where_expr_.program() != nullptr) {
      SValue where_tmp;
      VM::evaluate(
          txn_,
          where_expr_.program(),
          cur_buf_.size(),
          cur_buf_.data(),
          &where_tmp);

      where_pred = where_tmp.getBool();
    }

    if (where_pred) {
      for (int i = 0; i < select_list_.size(); ++i) {
        if (select_list_[i].rep_level >= cur_select_level_) {
          VM::accumulate(
              txn_,
              select_list_[i].compiled.program(),
              &select_list_[i].instance,
              cur_buf_.size(),
              cur_buf_.data());
        }
      }


      switch (aggr_strategy_) {

        case AggregationStrategy::AGGREGATE_ALL:
          break;

        case AggregationStrategy::AGGREGATE_WITHIN_RECORD_FLAT:
          if (next_level != 0) {
            break;
          }

        case AggregationStrategy::AGGREGATE_WITHIN_RECORD_DEEP:
          for (int i = 0; i < select_list_.size() && i < out_len; ++i) {
            VM::result(
                txn_,
                select_list_[i].compiled.program(),
                &select_list_[i].instance,
                &out[i]);

            VM::reset(
                txn_,
                select_list_[i].compiled.program(),
                &select_list_[i].instance);
          }

          row_ready = true;
          break;

        case AggregationStrategy::NO_AGGREGATION:
          for (int i = 0; i < select_list_.size() && i < out_len; ++i) {
            VM::evaluate(
                txn_,
                select_list_[i].compiled.program(),
                cur_buf_.size(),
                cur_buf_.data(),
                &out[i]);
          }

          row_ready = true;
          break;

      }

      cur_select_level_ = cur_fetch_level_;
    } else {
      cur_select_level_ = std::min(cur_select_level_, cur_fetch_level_);
    }

    for (const auto& col : columns_) {
      if (col.second.reader->maxRepetitionLevel() >= cur_select_level_) {
        cur_buf_[col.second.index] = SValue();
      }
    }

    if (row_ready) {
      return true;
    }
  }

  switch (aggr_strategy_) {
    case AggregationStrategy::AGGREGATE_ALL:
      for (int i = 0; i < select_list_.size() && i < out_len; ++i) {
        VM::result(
            txn_,
            select_list_[i].compiled.program(),
            &select_list_[i].instance,
            &out[i]);
      }

      return true;
  }

  return false;
}

bool CSTableScan::fetchNextWithoutColumns(SValue* row, int row_len) {
  csql::SValue sql_null;
  Vector<SValue> out_row(select_list_.size(), SValue{});

  while (cur_pos_ < num_records_) {
    ++cur_pos_;

    if (!filter_fn_()) {
      continue;
    }

    if (where_expr_.program() != nullptr) {
      SValue where_tmp;
      VM::evaluate(txn_, where_expr_.program(), 0, nullptr, &where_tmp);
      if (!where_tmp.getBool()) {
        continue;
      }
    }

    for (int i = 0; i < select_list_.size() && i < row_len; ++i) {
      VM::evaluate(
          txn_,
          select_list_[i].compiled.program(),
          1,
          &sql_null,
          &row[i]);
    }

    return true;
  }

  return false;
}


void CSTableScan::findColumns(
    RefPtr<ValueExpressionNode> expr,
    Set<String>* column_names) const {
  auto fieldref = dynamic_cast<ColumnReferenceNode*>(expr.get());
  if (fieldref != nullptr) {
    column_names->emplace(fieldref->fieldName());
  }

  for (const auto& e : expr->arguments()) {
    findColumns(e, column_names);
  }
}

uint64_t CSTableScan::findMaxRepetitionLevel(
    RefPtr<ValueExpressionNode> expr) const {
  uint64_t max_level = 0;

  auto fieldref = dynamic_cast<ColumnReferenceNode*>(expr.get());
  if (fieldref != nullptr) {
    auto col = columns_.find(fieldref->fieldName());
      if (col != columns_.end()) {
      auto col_level = col->second.reader->maxRepetitionLevel();
      if (col_level > max_level) {
        max_level = col_level;
      }
    }
  }

  for (const auto& e : expr->arguments()) {
    auto e_level = findMaxRepetitionLevel(e);
    if (e_level > max_level) {
      max_level = e_level;
    }
  }

  return max_level;
}

Vector<String> CSTableScan::columnNames() const {
  return column_names_;
}

size_t CSTableScan::getColumnCount() const {
  return column_names_.size();
}

SType CSTableScan::getColumnType(size_t idx) const {
  assert(idx < column_types_.size());
  return column_types_[idx];
}

size_t CSTableScan::rowsScanned() const {
  return rows_scanned_;
}

void CSTableScan::setFilter(Function<bool ()> filter_fn) {
  filter_fn_ = filter_fn;
}

void CSTableScan::setColumnType(String column, SType type) {
  const auto& col = columns_.find(column);
  if (col == columns_.end()) {
    return;
  }

  col->second.type = type;
}

CSTableScan::ColumnRef::ColumnRef(
    RefPtr<cstable::ColumnReader> r,
    size_t i,
    SType t) :
    reader(r),
    index(i),
    type(t) {}

CSTableScan::ExpressionRef::ExpressionRef(
    Transaction* _txn,
    size_t _rep_level,
    ValueExpression _compiled,
    ScratchMemory* smem) :
    txn(_txn),
    rep_level(_rep_level),
    compiled(std::move(_compiled)),
    instance(VM::allocInstance(txn, compiled.program(), smem)) {}

CSTableScan::ExpressionRef::ExpressionRef(
    ExpressionRef&& other) :
    rep_level(other.rep_level),
    compiled(std::move(other.compiled)),
    instance(other.instance) {
  other.instance.scratch = nullptr;
}

CSTableScan::ExpressionRef::~ExpressionRef() {
  if (instance.scratch) {
    VM::freeInstance(txn, compiled.program(), &instance);
  }
}

FastCSTableScan::FastCSTableScan(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<SequentialScanNode> stmt,
    const String& cstable_filename) :
    txn_(txn),
    execution_context_(execution_context),
    stmt_(stmt->deepCopyAs<SequentialScanNode>()),
    cstable_filename_(cstable_filename) {}

FastCSTableScan::FastCSTableScan(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<SequentialScanNode> stmt,
    RefPtr<cstable::CSTableReader> cstable,
    const String& cstable_filename /* = "<unknown>" */) :
    txn_(txn),
    execution_context_(execution_context),
    stmt_(stmt->deepCopyAs<SequentialScanNode>()),
    cstable_(cstable),
    cstable_filename_(cstable_filename) {}

FastCSTableScan::~FastCSTableScan() {}

ReturnCode FastCSTableScan::execute() {
  for (const auto& slnode : stmt_->selectList()) {
    select_list_.emplace_back(
        txn_->getCompiler()->buildValueExpression(txn_, slnode->expression()));
  }

  auto where_expr = stmt_->whereExpression();
  if (!where_expr.isEmpty()) {
    where_expr_ = txn_->getCompiler()->buildValueExpression(txn_, where_expr.get());
  }

  if (!cstable_.get()) {
    cstable_ = cstable::CSTableReader::openFile(cstable_filename_);
  }

  num_records_ = cstable_->numRecords();

  for (const auto& c : stmt_->selectedColumns()) {
    auto colinfo = stmt_->getComputedColumnInfo(c);
    column_types_.emplace_back(colinfo.second);
    column_buffers_.emplace_back(colinfo.second);

    if (cstable_->hasColumn(c)) {
      column_readers_.emplace_back(cstable_->getColumnReader(c));
    } else {
      column_readers_.emplace_back(nullptr);
    }
  }

  return ReturnCode::success();
}

ReturnCode FastCSTableScan::nextBatch(
    size_t limit,
    SVector* out,
    size_t* nrecords) {
  for (;;) {
    if (num_records_ == 0) {
      *nrecords = 0;
      return ReturnCode::success();
    }

    /* calculate batch size */
    size_t batch_size = std::min(
        num_records_,
        limit > 0 ? limit : size_t(8192 * 16));

    /* fetch input columns */
    for (size_t i = 0; i < column_buffers_.size(); ++i) {
      auto rc = ReturnCode::success();

      switch (column_types_[i]) {
        case SType::UINT64:
        case SType::TIMESTAMP64:
          rc = fetchColumnUInt64(i, batch_size);
          break;
        default:
          assert(0);
      }

      if (!rc.isSuccess()) {
        return rc;
      }
    }

    num_records_ -= batch_size;

    /* evalute where expression */
    std::vector<bool> filter_set;
    size_t filter_set_count = 0;
    if (where_expr_.program() == nullptr) {
      filter_set = std::vector<bool>(batch_size, true);
      filter_set_count = batch_size;
    } else {
      SValue pref(SType::BOOL);
      VM::evaluatePredicateVector(
          txn_,
          where_expr_.program(),
          column_buffers_.size(),
          column_buffers_.data(),
          batch_size,
          &filter_set,
          &filter_set_count);
    }

    if (filter_set_count == 0) {
      continue;
    }

    /* compute output columns */
    for (size_t i = 0; i < select_list_.size(); ++i) {
      VM::evaluateVector(
          txn_,
          select_list_[i].program(),
          column_buffers_.size(),
          column_buffers_.data(),
          batch_size,
          out + i,
          filter_set_count < batch_size ? &filter_set : nullptr);
    }

    *nrecords = filter_set_count;
    assert(*nrecords > 0);
    return ReturnCode::success();
  }
}

ReturnCode FastCSTableScan::fetchColumnUInt64(size_t idx, size_t batch_size) {
  auto buffer = &column_buffers_[idx];
  size_t buffer_len = batch_size * sizeof(uint64_t);
  if (buffer->getCapacity() < buffer_len) {
    buffer->increaseCapacity(buffer_len);
  }

  cstable::FixedColumnStorage col_storage(buffer->getMutableData(), &buffer_len);
  auto reader = column_readers_[idx];
  reader->readValues(batch_size, &col_storage);

  assert(buffer_len == batch_size * sizeof(uint64_t));
  buffer->setSize(buffer_len);

  return ReturnCode::success();
}

size_t FastCSTableScan::getColumnCount() const {
  return select_list_.size();
}

SType FastCSTableScan::getColumnType(size_t idx) const {
  assert(idx < select_list_.size());
  return select_list_[idx].getReturnType();
}

} // namespace csql

