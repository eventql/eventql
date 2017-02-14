/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <eventql/sql/result_list.h>

namespace csql {

ResultList::ResultList() {}

ResultList::ResultList(const Vector<String>& header) : columns_(header) {}

ResultList::ResultList(ResultList&& move) :
    columns_(std::move(move.columns_)),
    rows_(std::move(move.rows_)) {}

size_t ResultList::getNumColumns() const {
  return columns_.size();
}

size_t ResultList::getNumRows() const {
  return rows_.size();
}

const std::vector<std::string>& ResultList::getRow(size_t index) const {
  if (index >= rows_.size()) {
    RAISE(kRuntimeError, "invalid index");
  }

  return rows_[index];
}

const std::vector<std::string>& ResultList::getColumns() const {
  return columns_;
}

int ResultList::getComputedColumnIndex(const std::string& column_name) const {
  for (int i = 0; i < columns_.size(); ++i) {
    if (columns_[i] == column_name) {
      return i;
    }
  }

  return -1;
}

void ResultList::addHeader(const std::vector<std::string>& columns) {
  columns_ = columns;
}

void ResultList::addRow(const std::vector<std::string>& row) {
  rows_.emplace_back(row);
}

void ResultList::debugPrint() const {
  auto os = OutputStream::getStderr();
  debugPrint(os.get());
}

void ResultList::debugPrint(OutputStream* os) const {
  std::vector<int> col_widths;
  int col_min_width = 16;
  int total_width = 0;
  const int margin = 3;

  for (size_t i = 0; i < columns_.size(); ++i) {
    col_widths.push_back(
        std::max((int) columns_[i].size() + margin, col_min_width));
  }

  for (const auto& row : rows_) {
    auto row_len = std::min(columns_.size(), row.size());
    for (size_t i = 0; i < row_len; ++i) {
      // N.B. we count the actual string bytes instead of the n. of unicode
      // points. the worst thing that can happen is overshooting on the column
      // width
      col_widths[i] = std::max((int) row[i].size() + margin, col_widths[i]);
    }
  }

  for (const auto& col_width : col_widths) {
    total_width += col_width;
  }

  auto print_hsep = [os, &col_widths] () {
    for (int j = 0; j < col_widths.size(); ++j) {
      for (int i = 0; i < col_widths[j]; ++i) {
       char c = (i == 0) ? '+' : '-';
       os->printf("%c", c);
      }
    }
    os->printf("+\n");
  };

  auto print_row = [this, os, &col_widths] (const std::vector<std::string>& row) {
    for (int n = 0; n < columns_.size(); ++n) {
      auto val = n < row.size() ? row[n] : String("NULL");
      auto val_len = StringUtil::countUTF8CodePoints(val);
      os->printf("| %s", val.c_str());
      for (int i = col_widths[n] - val_len - 3; i >= 0; --i) {
        os->printf(" ");
      }
    }
    os->printf("|\n");
  };

  print_hsep();
  print_row(columns_);
  print_hsep();
  for (const auto& row : rows_) {
    print_row(row);
  }
  print_hsep();
}

}

