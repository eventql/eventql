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
#include "eventql/eventql.h"
#include <eventql/sql/drivers/csv/CSVTableScan.h>
#include <eventql/sql/runtime/QueryBuilder.h>

namespace csql {
namespace backends {
namespace csv {

CSVTableScan::CSVTableScan(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<SequentialScanNode> stmt,
    const std::vector<std::pair<std::string, SType>>& csv_columns,
    ScopedPtr<CSVInputStream> csv) :
    txn_(txn),
    execution_context_(execution_context),
    stmt_(stmt->deepCopyAs<SequentialScanNode>()),
    csv_columns_(csv_columns),
    csv_(std::move(csv)) {
  for (const auto& sl : stmt_->selectList()) {
    output_column_types_.emplace_back(sl->expression()->getReturnType());
  }
}

ReturnCode CSVTableScan::execute() {
  for (const auto& slnode : stmt_->selectList()) {
    select_list_.emplace_back(
        txn_->getCompiler()->buildValueExpression(txn_, slnode->expression()));
  }

  auto where_expr = stmt_->whereExpression();
  if (!where_expr.isEmpty()) {
    where_expr_ = txn_->getCompiler()->buildValueExpression(txn_, where_expr.get());
  }

  input_column_map_ = std::vector<size_t>(csv_columns_.size(), size_t(-1));

  auto selected_columns = stmt_->selectedColumns();
  for (size_t i = 0; i < selected_columns.size(); ++i) {

    for (size_t j = 0; j < csv_columns_.size(); ++j) {
      if (selected_columns[i]  == csv_columns_[j].first) {
        input_column_map_[j] = i;
        input_column_buffers_.emplace_back(csv_columns_[j].second);
        break;
      }
    }
  }

  if (input_column_buffers_.size() != selected_columns.size()) {
    return ReturnCode::error("EARG", "column list mismatch");
  }

  return ReturnCode::success();
}

ReturnCode CSVTableScan::nextBatch(
    SVector* out,
    size_t* nrecords) {
  for (;;) {
    size_t batch_size = 0;

    /* fetch input columns */
    auto rc = loadInputRows(&batch_size);
    if (!rc.isSuccess()) {
      return rc;
    }

    if (batch_size == 0) {
      *nrecords = 0;
      return ReturnCode::success();
    }

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
          where_expr_.program()->method_call,
          &vm_stack_,
          nullptr,
          input_column_buffers_.size(),
          input_column_buffers_.data(),
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
          select_list_[i].program()->method_call,
          &vm_stack_,
          nullptr,
          input_column_buffers_.size(),
          input_column_buffers_.data(),
          batch_size,
          out + i,
          filter_set_count < batch_size ? &filter_set : nullptr);
    }

    *nrecords = filter_set_count;
    assert(*nrecords > 0);
    return ReturnCode::success();
  }
}

ReturnCode CSVTableScan::loadInputRows(size_t* row_count) {
  *row_count = 0;

  for (auto& b : input_column_buffers_) {
    b.clear();
  }

  std::vector<String> inrow;
  while (csv_->readNextRow(&inrow)) {
    if (inrow.size() != input_column_map_.size()) {
      return ReturnCode::error("ERUNTIME", "inconsistent column count in CSV");
    }

    for (size_t i = 0; i < input_column_map_.size(); ++i) {
      if (input_column_map_[i] != size_t(-1)) {
        copyString(inrow[i], &input_column_buffers_[input_column_map_[i]]);
      }
    }

    ++(*row_count);
  }

  return ReturnCode::success();
}

size_t CSVTableScan::getColumnCount() const {
  return output_column_types_.size();
}

SType CSVTableScan::getColumnType(size_t idx) const {
  assert(idx < output_column_types_.size());
  return output_column_types_[idx];
}

} // namespace csv
} // namespace backends
} // namespace csql

