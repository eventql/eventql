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
#include <eventql/sql/table_expression.h>
#include <eventql/sql/backends/csv/CSVInputStream.h>

#include "eventql/eventql.h"

namespace csql {
namespace backends {
namespace csv {

struct CSVTableScan : public TableExpression {
public:

  CSVTableScan(
      const std::vector<std::pair<std::string, SType>> columns,
      ScopedPtr<CSVInputStream> csv);

  ReturnCode execute() override;
  ReturnCode nextBatch(SVector* columns, size_t* len) override;

  size_t getColumnCount() const override;
  SType getColumnType(size_t idx) const override;

protected:
  std::vector<std::pair<std::string, SType>> columns_;
  ScopedPtr<CSVInputStream> csv_;
};

} // namespace csv
} // namespace backends
} // namespace csql
