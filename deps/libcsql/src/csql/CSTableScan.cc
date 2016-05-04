/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/CSTableScan.h>
#include <csql/qtree/ColumnReferenceNode.h>
#include <csql/runtime/defaultruntime.h>
#include <csql/runtime/compiler.h>
#include <stx/ieee754.h>
#include <stx/logging.h>

using namespace stx;

namespace csql {

CSTableScan::CSTableScan(
    Transaction* txn,
    RefPtr<SequentialScanNode> stmt,
    const String& cstable_filename,
    QueryBuilder* runtime) :
    txn_(txn),
    stmt_(stmt->deepCopyAs<SequentialScanNode>()),
    cstable_filename_(cstable_filename),
    runtime_(runtime),
    colindex_(0),
    aggr_strategy_(stmt_->aggregationStrategy()),
    rows_scanned_(0),
    opened_(false) {
  column_names_ = stmt_->outputColumns();
}

CSTableScan::CSTableScan(
    Transaction* txn,
    RefPtr<SequentialScanNode> stmt,
    RefPtr<cstable::CSTableReader> cstable,
    QueryBuilder* runtime) :
    txn_(txn),
    stmt_(stmt->deepCopyAs<SequentialScanNode>()),
    cstable_(cstable),
    runtime_(runtime),
    colindex_(0),
    aggr_strategy_(stmt_->aggregationStrategy()),
    rows_scanned_(0),
    opened_(false) {
  column_names_ = stmt_->outputColumns();
}

void CSTableScan::open() {
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
        runtime_->buildValueExpression(txn_, slnode->expression()),
        &scratch_);
  }

  if (!where_expr.isEmpty()) {
    resolveColumns(where_expr.get());
    where_expr_ = runtime_->buildValueExpression(txn_, where_expr.get());
  }
}

bool CSTableScan::nextRow(SValue* out, int out_len) {
  return false;
}

//void CSTableScan::onInputsReady() {
//  logTrace("sql", "Scanning cstable: $0", cstable_filename_);
//
//  if (!opened_) {
//    open();
//  }
//
//  if (columns_.empty()) {
//    scanWithoutColumns();
//  } else {
//    scan();
//  }
//
//  //txn_->incrNumSubtasksCompleted(1);
//}

//void CSTableScan::scan() {
//  uint64_t select_level = 0;
//  uint64_t fetch_level = 0;
//  bool filter_pred = true;
//
//  Vector<SValue> in_row(colindex_, SValue{});
//  Vector<SValue> out_row(select_list_.size(), SValue{});
//
//  size_t num_records = 0;
//  size_t total_records = cstable_->numRecords();
//  while (num_records < total_records) {
//    ++rows_scanned_;
//    uint64_t next_level = 0;
//
//    if (fetch_level == 0) {
//      if (num_records < total_records && filter_fn_) {
//        filter_pred = filter_fn_();
//      }
//    }
//
//    for (auto& col : columns_) {
//      auto nextr = col.second.reader->nextRepetitionLevel();
//
//      if (nextr >= fetch_level) {
//        auto& reader = col.second.reader;
//
//        uint64_t r;
//        uint64_t d;
//
//        switch (col.second.reader->type()) {
//
//          case cstable::ColumnType::STRING: {
//            String v;
//            reader->readString(&r, &d, &v);
//
//            if (d < reader->maxDefinitionLevel()) {
//              in_row[col.second.index] = SValue();
//            } else {
//              switch (col.second.type) {
//                case SQL_NULL:
//                  in_row[col.second.index] = SValue::newNull();
//                  break;
//                case SQL_STRING:
//                  in_row[col.second.index] = SValue::newString(v);
//                  break;
//                case SQL_FLOAT:
//                  in_row[col.second.index] = SValue::newFloat(v);
//                  break;
//                case SQL_INTEGER:
//                  in_row[col.second.index] = SValue::newInteger(v);
//                  break;
//                case SQL_BOOL:
//                  in_row[col.second.index] = SValue::newBool(v);
//                  break;
//                case SQL_TIMESTAMP:
//                  in_row[col.second.index] = SValue::newTimestamp(v);
//                  break;
//              }
//            }
//
//            break;
//          }
//
//          case cstable::ColumnType::UNSIGNED_INT: {
//            uint64_t v = 0;
//            reader->readUnsignedInt(&r, &d, &v);
//
//            if (d < reader->maxDefinitionLevel()) {
//              in_row[col.second.index] = SValue();
//            } else {
//              switch (col.second.type) {
//                case SQL_NULL:
//                  in_row[col.second.index] = SValue::newNull();
//                  break;
//                case SQL_STRING:
//                  in_row[col.second.index] = SValue::newInteger(v).toString();
//                  break;
//                case SQL_FLOAT:
//                  in_row[col.second.index] = SValue::newFloat(v);
//                  break;
//                case SQL_INTEGER:
//                  in_row[col.second.index] = SValue::newInteger(v);
//                  break;
//                case SQL_BOOL:
//                  in_row[col.second.index] = SValue::newBool(v);
//                  break;
//                case SQL_TIMESTAMP:
//                  in_row[col.second.index] = SValue::newTimestamp(v);
//                  break;
//              }
//            }
//
//            break;
//          }
//
//          case cstable::ColumnType::SIGNED_INT: {
//            int64_t v = 0;
//            reader->readSignedInt(&r, &d, &v);
//
//            if (d < reader->maxDefinitionLevel()) {
//              in_row[col.second.index] = SValue();
//            } else {
//              switch (col.second.type) {
//                case SQL_NULL:
//                  in_row[col.second.index] = SValue::newNull();
//                  break;
//                case SQL_STRING:
//                  in_row[col.second.index] = SValue::newInteger(v).toString();
//                  break;
//                case SQL_FLOAT:
//                  in_row[col.second.index] = SValue::newFloat(v);
//                  break;
//                case SQL_INTEGER:
//                  in_row[col.second.index] = SValue::newInteger(v);
//                  break;
//                case SQL_BOOL:
//                  in_row[col.second.index] = SValue::newBool(v);
//                  break;
//                case SQL_TIMESTAMP:
//                  in_row[col.second.index] = SValue::newTimestamp(v);
//                  break;
//              }
//            }
//
//            break;
//          }
//
//          case cstable::ColumnType::BOOLEAN: {
//            bool v = 0;
//            reader->readBoolean(&r, &d, &v);
//
//            if (d < reader->maxDefinitionLevel()) {
//              in_row[col.second.index] = SValue(SValue::BoolType(false));
//            } else {
//              switch (col.second.type) {
//                case SQL_NULL:
//                  in_row[col.second.index] = SValue::newNull();
//                  break;
//                case SQL_STRING:
//                  in_row[col.second.index] = SValue::newBool(v).toString();
//                  break;
//                case SQL_FLOAT:
//                  in_row[col.second.index] = SValue::newFloat(v);
//                  break;
//                case SQL_INTEGER:
//                  in_row[col.second.index] = SValue::newInteger(v);
//                  break;
//                case SQL_BOOL:
//                  in_row[col.second.index] = SValue::newBool(v);
//                  break;
//                case SQL_TIMESTAMP:
//                  in_row[col.second.index] = SValue::newTimestamp(v);
//                  break;
//              }
//            }
//
//            break;
//          }
//
//          case cstable::ColumnType::FLOAT: {
//            double v = 0;
//            reader->readFloat(&r, &d, &v);
//
//            if (d < reader->maxDefinitionLevel()) {
//              in_row[col.second.index] = SValue();
//            } else {
//              switch (col.second.type) {
//                case SQL_NULL:
//                  in_row[col.second.index] = SValue::newNull();
//                  break;
//                case SQL_STRING:
//                  in_row[col.second.index] = SValue::newFloat(v).toString();
//                  break;
//                case SQL_FLOAT:
//                  in_row[col.second.index] = SValue::newFloat(v);
//                  break;
//                case SQL_INTEGER:
//                  in_row[col.second.index] = SValue::newInteger(v);
//                  break;
//                case SQL_BOOL:
//                  in_row[col.second.index] = SValue::newBool(v);
//                  break;
//                case SQL_TIMESTAMP:
//                  in_row[col.second.index] = SValue::newTimestamp(v);
//                  break;
//              }
//            }
//
//            break;
//          }
//
//          case cstable::ColumnType::DATETIME: {
//            UnixTime v;
//            reader->readDateTime(&r, &d, &v);
//
//            if (d < reader->maxDefinitionLevel()) {
//              in_row[col.second.index] = SValue();
//            } else {
//              switch (col.second.type) {
//                case SQL_NULL:
//                  in_row[col.second.index] = SValue::newNull();
//                  break;
//                case SQL_STRING:
//                  in_row[col.second.index] = SValue::newTimestamp(v).toString();
//                  break;
//                case SQL_FLOAT:
//                  in_row[col.second.index] = SValue::newTimestamp(v).toFloat();
//                  break;
//                case SQL_INTEGER:
//                  in_row[col.second.index] = SValue::newTimestamp(v).toInteger();
//                  break;
//                case SQL_TIMESTAMP:
//                  in_row[col.second.index] = SValue::newTimestamp(v);
//                  break;
//                default:
//                  RAISE(kIllegalStateError);
//              }
//            }
//
//            break;
//          }
//
//          case cstable::ColumnType::SUBRECORD:
//            RAISE(kIllegalStateError);
//
//        }
//      }
//
//      next_level = std::max(
//          next_level,
//          col.second.reader->nextRepetitionLevel());
//    }
//
//    fetch_level = next_level;
//    if (fetch_level == 0) {
//      ++num_records;
//    }
//
//    bool where_pred = filter_pred;
//    if (where_pred && where_expr_.program() != nullptr) {
//      SValue where_tmp;
//      VM::evaluate(
//          txn_,
//          where_expr_.program(),
//          in_row.size(),
//          in_row.data(),
//          &where_tmp);
//
//      where_pred = where_tmp.getBool();
//    }
//
//    if (where_pred) {
//      for (int i = 0; i < select_list_.size(); ++i) {
//        if (select_list_[i].rep_level >= select_level) {
//          VM::accumulate(
//              txn_,
//              select_list_[i].compiled.program(),
//              &select_list_[i].instance,
//              in_row.size(),
//              in_row.data());
//        }
//      }
//
//      switch (aggr_strategy_) {
//
//        case AggregationStrategy::AGGREGATE_ALL:
//          break;
//
//        case AggregationStrategy::AGGREGATE_WITHIN_RECORD_FLAT:
//          if (next_level != 0) {
//            break;
//          }
//
//        case AggregationStrategy::AGGREGATE_WITHIN_RECORD_DEEP:
//          for (int i = 0; i < select_list_.size(); ++i) {
//            VM::result(
//                txn_,
//                select_list_[i].compiled.program(),
//                &select_list_[i].instance,
//                &out_row[i]);
//
//            VM::reset(
//                txn_,
//                select_list_[i].compiled.program(),
//                &select_list_[i].instance);
//          }
//
//          if (!output_(out_row.data(), out_row.size())) {
//            return;
//          }
//
//          break;
//
//        case AggregationStrategy::NO_AGGREGATION:
//          for (int i = 0; i < select_list_.size(); ++i) {
//            VM::evaluate(
//                txn_,
//                select_list_[i].compiled.program(),
//                in_row.size(),
//                in_row.data(),
//                &out_row[i]);
//          }
//
//          if (!output_(out_row.data(), out_row.size())) {
//            return;
//          }
//
//          break;
//
//      }
//
//      select_level = fetch_level;
//    } else {
//      select_level = std::min(select_level, fetch_level);
//    }
//
//    for (const auto& col : columns_) {
//      if (col.second.reader->maxRepetitionLevel() >= select_level) {
//        in_row[col.second.index] = SValue();
//      }
//    }
//  }
//
//  switch (aggr_strategy_) {
//    case AggregationStrategy::AGGREGATE_ALL:
//      for (int i = 0; i < select_list_.size(); ++i) {
//        VM::result(
//            txn_,
//            select_list_[i].compiled.program(),
//            &select_list_[i].instance,
//            &out_row[i]);
//      }
//
//      output_(out_row.data(), out_row.size());
//      break;
//
//    default:
//      break;
//
//  }
//}

//void CSTableScan::scanWithoutColumns() {
//  Vector<SValue> out_row(select_list_.size(), SValue{});
//
//  size_t total_records = cstable_->numRecords();
//  for (size_t i = 0; i < total_records; ++i) {
//    bool where_pred = true;
//    if (where_expr_.program() != nullptr) {
//      SValue where_tmp;
//      VM::evaluate(txn_, where_expr_.program(), 0, nullptr, &where_tmp);
//      where_pred = where_tmp.getBool();
//    }
//
//    if (where_pred) {
//      switch (aggr_strategy_) {
//
//        case AggregationStrategy::AGGREGATE_ALL:
//          for (int i = 0; i < select_list_.size(); ++i) {
//            VM::accumulate(
//                txn_,
//                select_list_[i].compiled.program(),
//                &select_list_[i].instance,
//                0,
//                nullptr);
//          }
//          break;
//
//        case AggregationStrategy::AGGREGATE_WITHIN_RECORD_DEEP:
//        case AggregationStrategy::AGGREGATE_WITHIN_RECORD_FLAT:
//        case AggregationStrategy::NO_AGGREGATION:
//          for (int i = 0; i < select_list_.size(); ++i) {
//            VM::evaluate(
//                txn_,
//                select_list_[i].compiled.program(),
//                0,
//                nullptr,
//                &out_row[i]);
//          }
//
//          if (!output_(out_row.data(), out_row.size())) {
//            return;
//          }
//          break;
//      }
//    }
//  }
//
//  switch (aggr_strategy_) {
//    case AggregationStrategy::AGGREGATE_ALL:
//      for (int i = 0; i < select_list_.size(); ++i) {
//        VM::result(
//            txn_,
//            select_list_[i].compiled.program(),
//            &select_list_[i].instance,
//            &out_row[i]);
//      }
//
//      output_(out_row.data(), out_row.size());
//      break;
//
//    default:
//      break;
//
//  }
//}
//
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

Option<SHA1Hash> CSTableScan::cacheKey() const {
  return cache_key_;
}

void CSTableScan::setCacheKey(const SHA1Hash& key) {
  cache_key_ = Some(key);
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
