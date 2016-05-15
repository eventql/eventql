/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <eventql/sql/CSTableScan.h>
#include <eventql/sql/qtree/ColumnReferenceNode.h>
#include <eventql/sql/runtime/defaultruntime.h>
#include <eventql/sql/runtime/compiler.h>
#include <eventql/util/ieee754.h>
#include <eventql/util/logging.h>

using namespace stx;

namespace csql {

CSTableScan::CSTableScan(
    Transaction* txn,
    RefPtr<SequentialScanNode> stmt,
    const String& cstable_filename) :
    txn_(txn),
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
  column_names_ = stmt_->outputColumns();
}

CSTableScan::CSTableScan(
    Transaction* txn,
    RefPtr<SequentialScanNode> stmt,
    RefPtr<cstable::CSTableReader> cstable) :
    txn_(txn),
    stmt_(stmt->deepCopyAs<SequentialScanNode>()),
    cstable_(cstable),
    colindex_(0),
    aggr_strategy_(stmt_->aggregationStrategy()),
    rows_scanned_(0),
    num_records_(0),
    opened_(false),
    cur_select_level_(0),
    cur_fetch_level_(0),
    cur_filter_pred_(true),
    cur_pos_(0) {
  column_names_ = stmt_->outputColumns();
}

ScopedPtr<ResultCursor> CSTableScan::execute() {
  if (!opened_) {
    open();
  }

  cur_buf_.resize(colindex_);
  num_records_ = cstable_->numRecords();

  return mkScoped(
      new DefaultResultCursor(
          select_list_.size(),
          std::bind(
              &CSTableScan::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}


void CSTableScan::open() {
  auto runtime = txn_->getCompiler();
  opened_ = true;

  if (cstable_.get() == nullptr) {
    cstable_ = cstable::CSTableReader::openFile(cstable_filename_);
  }

  Set<String> column_names;
  for (const auto& slnode : stmt_->selectList()) {
    findColumns(slnode->expression(), &column_names);
  }

  auto where_expr = stmt_->whereExpression();
  if (!where_expr.isEmpty()) {
    findColumns(where_expr.get(), &column_names);
  }

  for (const auto& col : column_names) {
    if (!cstable_->hasColumn(col)) {
      continue;
    }

    auto reader = cstable_->getColumnReader(col);

    sql_type type;
    switch (reader->type()) {
      case cstable::ColumnType::STRING:
        type = SQL_STRING;
        break;
      case cstable::ColumnType::SIGNED_INT:
      case cstable::ColumnType::UNSIGNED_INT:
        type = SQL_INTEGER;
        break;
      case cstable::ColumnType::FLOAT:
        type = SQL_FLOAT;
        break;
      case cstable::ColumnType::BOOLEAN:
        type = SQL_BOOL;
        break;
      case cstable::ColumnType::DATETIME:
        type = SQL_TIMESTAMP;
        break;
      case cstable::ColumnType::SUBRECORD:
        RAISE(kIllegalStateError);
    }

    columns_.emplace(col, ColumnRef(reader, colindex_++, type));
  }

  for (const auto& slnode : stmt_->selectList()) {
    resolveColumns(slnode->expression());

    select_list_.emplace_back(
        txn_,
        findMaxRepetitionLevel(slnode->expression()),
        runtime->buildValueExpression(txn_, slnode->expression()),
        &scratch_);
  }

  if (!where_expr.isEmpty()) {
    resolveColumns(where_expr.get());
    where_expr_ = runtime->buildValueExpression(txn_, where_expr.get());
  }
}

bool CSTableScan::next(SValue* out, int out_len) {
  if (columns_.empty()) {
    return fetchNextWithoutColumns(out, out_len);
  } else {
    return fetchNext(out, out_len);
  }
}

//void CSTableScan::onInputsReady() {
//  logTrace("sql", "Scanning cstable: $0", cstable_filename_);
//
//  if (!opened_) {
//    open();
//  }
//
//
//  //txn_->incrNumSubtasksCompleted(1);
//}

bool CSTableScan::fetchNext(SValue* out, int out_len) {
  if (cur_pos_ >= num_records_) {
    return false;
  }

  while (cur_pos_ < num_records_) {
    ++rows_scanned_;
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
                case SQL_NULL:
                  cur_buf_[col.second.index] = SValue::newNull();
                  break;
                case SQL_STRING:
                  cur_buf_[col.second.index] = SValue::newString(v);
                  break;
                case SQL_FLOAT:
                  cur_buf_[col.second.index] = SValue::newFloat(v);
                  break;
                case SQL_INTEGER:
                  cur_buf_[col.second.index] = SValue::newInteger(v);
                  break;
                case SQL_BOOL:
                  cur_buf_[col.second.index] = SValue::newBool(v);
                  break;
                case SQL_TIMESTAMP:
                  cur_buf_[col.second.index] = SValue::newTimestamp(v);
                  break;
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
                case SQL_NULL:
                  cur_buf_[col.second.index] = SValue::newNull();
                  break;
                case SQL_STRING:
                  cur_buf_[col.second.index] = SValue::newInteger(v).toString();
                  break;
                case SQL_FLOAT:
                  cur_buf_[col.second.index] = SValue::newFloat(v);
                  break;
                case SQL_INTEGER:
                  cur_buf_[col.second.index] = SValue::newInteger(v);
                  break;
                case SQL_BOOL:
                  cur_buf_[col.second.index] = SValue::newBool(v);
                  break;
                case SQL_TIMESTAMP:
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
                case SQL_NULL:
                  cur_buf_[col.second.index] = SValue::newNull();
                  break;
                case SQL_STRING:
                  cur_buf_[col.second.index] = SValue::newInteger(v).toString();
                  break;
                case SQL_FLOAT:
                  cur_buf_[col.second.index] = SValue::newFloat(v);
                  break;
                case SQL_INTEGER:
                  cur_buf_[col.second.index] = SValue::newInteger(v);
                  break;
                case SQL_BOOL:
                  cur_buf_[col.second.index] = SValue::newBool(v);
                  break;
                case SQL_TIMESTAMP:
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
                case SQL_NULL:
                  cur_buf_[col.second.index] = SValue::newNull();
                  break;
                case SQL_STRING:
                  cur_buf_[col.second.index] = SValue::newBool(v).toString();
                  break;
                case SQL_FLOAT:
                  cur_buf_[col.second.index] = SValue::newFloat(v);
                  break;
                case SQL_INTEGER:
                  cur_buf_[col.second.index] = SValue::newInteger(v);
                  break;
                case SQL_BOOL:
                  cur_buf_[col.second.index] = SValue::newBool(v);
                  break;
                case SQL_TIMESTAMP:
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
                case SQL_NULL:
                  cur_buf_[col.second.index] = SValue::newNull();
                  break;
                case SQL_STRING:
                  cur_buf_[col.second.index] = SValue::newFloat(v).toString();
                  break;
                case SQL_FLOAT:
                  cur_buf_[col.second.index] = SValue::newFloat(v);
                  break;
                case SQL_INTEGER:
                  cur_buf_[col.second.index] = SValue::newInteger(v);
                  break;
                case SQL_BOOL:
                  cur_buf_[col.second.index] = SValue::newBool(v);
                  break;
                case SQL_TIMESTAMP:
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
                case SQL_NULL:
                  cur_buf_[col.second.index] = SValue::newNull();
                  break;
                case SQL_STRING:
                  cur_buf_[col.second.index] = SValue::newTimestamp(v).toString();
                  break;
                case SQL_FLOAT:
                  cur_buf_[col.second.index] = SValue::newTimestamp(v).toFloat();
                  break;
                case SQL_INTEGER:
                  cur_buf_[col.second.index] = SValue::newTimestamp(v).toInteger();
                  break;
                case SQL_TIMESTAMP:
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

    default:
      RAISE(kIllegalStateError, "invalid aggregation strategy");

  }
}

bool CSTableScan::fetchNextWithoutColumns(SValue* row, int row_len) {
  Vector<SValue> out_row(select_list_.size(), SValue{});

  while (cur_pos_ < num_records_) {
    ++cur_pos_;

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
          0,
          nullptr,
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

void CSTableScan::resolveColumns(RefPtr<ValueExpressionNode> expr) const {
  auto fieldref = dynamic_cast<ColumnReferenceNode*>(expr.get());
  if (fieldref != nullptr) {
    auto colname = fieldref->fieldName();
    auto col = columns_.find(colname);
    if (col == columns_.end()) {
      RAISEF(kNotFoundError, "column(s) not found: $0", colname);
    } else {
      fieldref->setColumnIndex(col->second.index);
    }
  }

  for (const auto& e : expr->arguments()) {
    resolveColumns(e);
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

size_t CSTableScan::numColumns() const {
  return column_names_.size();
}

size_t CSTableScan::rowsScanned() const {
  return rows_scanned_;
}

void CSTableScan::setFilter(Function<bool ()> filter_fn) {
  filter_fn_ = filter_fn;
}

void CSTableScan::setColumnType(String column, sql_type type) {
  const auto& col = columns_.find(column);
  if (col == columns_.end()) {
    return;
  }

  col->second.type = type;
}

CSTableScan::ColumnRef::ColumnRef(
    RefPtr<cstable::ColumnReader> r,
    size_t i,
    sql_type t) :
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


} // namespace csql
