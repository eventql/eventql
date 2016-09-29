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
#include <eventql/util/stdtypes.h>
#include <eventql/util/csv/CSVInputStream.h>
#include <eventql/util/exception.h>
#include <eventql/sql/parser/astnode.h>
#include <eventql/sql/backends/csv/csvtableref.h>
#include <eventql/sql/tasks/tablescan.h>
#include <eventql/sql/svalue.h>

namespace csql {
namespace csv_backend {

CSVTableRef::CSVTableRef(
    std::unique_ptr<CSVInputStream>&& csv,
    bool headers /* = false */) :
    csv_(std::move(csv)),
    num_cols_(-1),
    min_cols_(0),
    row_index_(0),
    dirty_(false) {
  if (headers) {
    readHeaders();
  }
}

std::vector<std::string> CSVTableRef::columns() {
  std::vector<std::string> cols;

  if (min_cols_ > 0) {
    if (headers_.size() > 0) {
      for (const auto& header : headers_) {
        cols.emplace_back(header.first);
      }
    } else  {
      for (int i = 0; i < min_cols_; ++i) {
        cols.emplace_back("col" + std::to_string(i + 1));
      }
    }
  }

  return cols;
}

int CSVTableRef::getComputedColumnIndex(const std::string& name) {
  const auto& header = headers_.find(name);

  if (header != headers_.end()) {
    return header->second;
  }

  if (name.size() > 3 &&
      strncmp(name.c_str(), "col", 3) == 0) {
    int index = -1;

    try {
      index = std::stoi(std::string(name.c_str() + 3, name.size() - 3));
    } catch (std::exception e) {
      return -1;
    }

    if (index > 0) {
      return index - 1;
    }
  }

  return -1;
}

std::string CSVTableRef::getColumnName(int index) {
  for (const auto& header : headers_) {
    if (header.second == index) {
      return header.first;
    }
  }

  RAISE(kIndexError, "no such column");
}

void CSVTableRef::executeScan(TableScan* scan) {
  if (dirty_) {
    rewind();
  }

  for (;;) {
    std::vector<SValue> row;

    if (!readNextRow(&row)) {
      break;
    }

    if (!scan->nextRow(row.data(), row.size())) {
      break;
    }
  }
}

bool CSVTableRef::readNextRow(std::vector<SValue>* target) {
  std::vector<std::string> row;
  dirty_ = true;

  if (!csv_->readNextRow(&row) || row.size() == 0) {
    return false;
  } else {
    row_index_++;
  }

  if (row.size() < min_cols_) {
    RAISE(
        kRuntimeError,
        "error while reading CSV file '%s': "
        "row #%i does not have enough columns; columns found=%i, required=%i\n",
        csv_->getInputStream().getFileName().c_str(),
        row_index_,
        row.size(),
        min_cols_); // FIXPAUL filename
  }

  if (num_cols_ == -1) {
    num_cols_ = row.size();
  } else if (row.size() != num_cols_) {
    RAISE(
        kRuntimeError,
        "error while reading CSV file '%s': "
        "csv row #%i does not have the same number of columns as the previous "
        "line -- number of columns found=%i, previous=%i\n",
        csv_->getInputStream().getFileName().c_str(),
        row_index_,
        row.size(),
        num_cols_); // FIXPAUL filename
  }

  for (const auto& col : row) {
    target->emplace_back(col);
  }

  return true;
}

void CSVTableRef::readHeaders() {
  std::vector<std::string> headers;

  if (!csv_->readNextRow(&headers) || headers.size() == 0) {
    RAISE(
        kRuntimeError,
        "no headers found in CSV file '%s'",
        csv_->getInputStream().getFileName().c_str());
  }

  size_t col_index = 0;
  for (const auto& header : headers) {
    headers_.emplace(header, col_index++);
  }

  num_cols_ = col_index;
  min_cols_ = col_index;
  row_index_++;
}

void CSVTableRef::rewind() {
  csv_->rewind();

  if (headers_.size() > 0) {
    if (!csv_->skipNextRow()) {
      RAISE(
        kRuntimeError,
        "CSV file '%s' changed while we were reading it",
        csv_->getInputStream().getFileName().c_str());
    }

    row_index_ = 1;
  } else {
    row_index_ = 0;
  }
}

}
}
