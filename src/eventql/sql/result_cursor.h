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
#include <eventql/util/autoref.h>
#include <eventql/util/SHA1.h>
#include <eventql/sql/svalue.h>

#include "eventql/eventql.h"

namespace csql {
class TableExpression;

class ResultCursor {
public:

  ResultCursor();
  ResultCursor(ScopedPtr<TableExpression> table_expression);

  /**
   * Fetch the next row from the cursor. Returns true if a row was returned
   * into the provided storage and false if the last row of the query has been
   * read (EOF). If this method returns false the provided storage will remain
   * unchanged.
   *
   * This method will block until the next row is available.
   */
  bool next(SValue* row);

  size_t getColumnCount() const;
  SType getColumnType(size_t idx) const;

protected:
  ScopedPtr<TableExpression> table_expression_;
  bool started_;
};

} // namespace csql

