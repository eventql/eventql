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
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/SHA1.h>
#include <eventql/util/return_code.h>
#include <eventql/sql/svalue.h>

namespace csql {
class TableExpression;

class ResultCursor {
public:

  ResultCursor();
  ResultCursor(ScopedPtr<TableExpression> table_expression);

  /**
   * Returns true if the cursor is pointing to a valid row
   */
  bool isValid() const;

  /**
   * Fetch the next row from the cursor. This method will block until the next
   * row is available.
   */
  ReturnCode next();

  size_t getColumnCount() const;
  SType getColumnType(size_t idx) const;
  const std::string& getColumnName(size_t idx) const;

  std::string getColumnString(size_t idx) const;
  void* getColumnData(size_t idx) const;

protected:
  ScopedPtr<TableExpression> table_expression_;
  bool started_;
  bool eof_;
  std::vector<SVector> buffer_;
  size_t buffer_len_;
};

} // namespace csql

