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

#ifndef _FNORDMETRIC_CSVTABLEREF_H
#define _FNORDMETRIC_CSVTABLEREF_H
#include <memory>
#include <unordered_map>
#include <string>
#include <eventql/util/csv/CSVInputStream.h>
#include <eventql/sql/backends/tableref.h>

#include "eventql/eventql.h"

namespace csql {
class SValue;
namespace csv_backend {

/**
 * A CSVTableRef instance is threadfriendly but not threadsafe. You must
 * synchronize access to all methods.
 */
class CSVTableRef : public TableRef {
public:
  CSVTableRef(
      std::unique_ptr<CSVInputStream>&& csv,
      bool headers = false);

  std::vector<std::string> columns() override;
  int getComputedColumnIndex(const std::string& name) override;
  std::string getColumnName(int index) override;
  void executeScan(TableScan* scan) override;

  bool readNextRow(std::vector<SValue>* target);
  void readHeaders();
  void rewind();

protected:
  std::unordered_map<std::string, size_t> headers_;
  std::unique_ptr<CSVInputStream> csv_;
  int num_cols_;
  int min_cols_;
  int row_index_;
  bool dirty_;
};

}
}
#endif
