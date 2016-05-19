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
#ifndef _FNORDMETRIC_QUERY_RESULTLIST_H
#define _FNORDMETRIC_QUERY_RESULTLIST_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include <eventql/sql/svalue.h>

namespace csql {

class ResultList{
public:

  ResultList() {}
  ResultList(const Vector<String>& header) : columns_(header) {}
  ResultList(const ResultList& copy) = delete;
  ResultList& operator=(const ResultList& copy) = delete;

  ResultList(ResultList&& move) :
      columns_(std::move(move.columns_)),
      rows_(std::move(move.rows_)) {}

  size_t getNumColumns() const {
    return columns_.size();
  }

  size_t getNumRows() const {
    return rows_.size();
  }

  const std::vector<std::string>& getRow(size_t index) const {
    if (index >= rows_.size()) {
      RAISE(kRuntimeError, "invalid index");
    }

    return rows_[index];
  }

  const std::vector<std::string>& getColumns() const {
    return columns_;
  }

  int getComputedColumnIndex(const std::string& column_name) const {
    for (int i = 0; i < columns_.size(); ++i) {
      if (columns_[i] == column_name) {
        return i;
      }
    }

    return -1;
  }

  void addHeader(const std::vector<std::string>& columns) {
    columns_ = columns;
  }

  bool addRow(const csql::SValue* row, int row_len) {
    Vector<String> str_row;
    for (int i = 0; i < row_len; ++i) {
      str_row.emplace_back(row[i].getString());
    }

    rows_.emplace_back(str_row);
    return true;
  }

  void debugPrint() const {
    auto os = OutputStream::getStderr();
    debugPrint(os.get());
  }

  void debugPrint(OutputStream* os) const {
    std::vector<int> col_widths;
    int total_width = 0;

    for (size_t i = 0; i < columns_.size(); ++i) {
      col_widths.push_back(20);
      total_width += 20;
    }

    auto print_hsep = [os, &col_widths] () {
      for (auto w : col_widths) {
        for (int i = 0; i < w; ++i) {
          char c = (i == 0 || i == w - 1) ? '+' : '-';
          os->printf("%c", c);
        }
      }
      os->printf("\n");
    };

    auto print_row = [this, os, &col_widths] (const std::vector<std::string>& row) {
      for (int n = 0; n < columns_.size(); ++n) {
        auto val = n < row.size() ? row[n] : String("NULL");
        os->printf("| %s", val.c_str());
        for (int i = col_widths[n] - val.size() - 3; i > 0; --i) {
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

protected:
  std::vector<std::string> columns_;
  std::vector<std::vector<std::string>> rows_;
};

}
#endif
