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
#include <eventql/sql/svalue.h>

namespace csql {

class ResultList{
public:

  ResultList();
  ResultList(const Vector<String>& header);
  ResultList(const ResultList& copy) = delete;
  ResultList& operator=(const ResultList& copy) = delete;

  ResultList(ResultList&& move);

  size_t getNumColumns() const;
  size_t getNumRows() const;

  const std::vector<std::string>& getRow(size_t index) const;
  const std::vector<std::string>& getColumns() const;

  int getComputedColumnIndex(const std::string& column_name) const;

  void addHeader(const std::vector<std::string>& columns);
  void addRow(const std::vector<std::string>& row);

  void debugPrint() const;
  void debugPrint(OutputStream* os) const;

protected:
  std::vector<std::string> columns_;
  std::vector<std::vector<std::string>> rows_;
};

}
