/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#include <eventql/util/option.h>
#include <eventql/sql/qtree/QueryTreeNode.h>

#include "eventql/eventql.h"

namespace csql {
class Transaction;

struct QualifiedColumn {
  String qualified_name;
  String short_name;
};

/**
 * A table expression node describes an executable table expression. One way
 * to see the TableExpressionNode is as a builder/config object for the actual
 * table expression implementation.
 *
 * Each TableExpressionNode manages three sets of column lists.
 *
 *   - The AVAILABLE COLUMNS include all columns that can be referenced on
 *     the table expression
 *
 *   - the COMPUTED COLUMNS include all columns that are actually referenced
 *     by an upstream table expression or the query and need to be calculated
 *     and returnd
 *
 *   - the OUTPUT COLUMNS are the subset of the computed columns that should be
 *     returned to the user
 *
 */
class TableExpressionNode : public QueryTreeNode {
public:

  /**
   * Returns all columns that may be referenced on this table expression. This
   * list is equal to or a superset of the set of actual output columns. I.e. it
   * includes all columns that _could_ be referenced on this tables as opposed to
   * the output columns, which returns all columns that actually are referenced
   * and need to be returned on execution
   */
  virtual Vector<QualifiedColumn> allColumns() const = 0; //getAvailableColumns

  /**
   * Returns the nominal output column list. I.e. the names of all output columns
   * that this table expression will return and that should be displayed to the
   * user
   */
  virtual Vector<String> outputColumns() const = 0; // getResultColumns

  /**
   * Returns the number of computed columns that this table expression will return.
   * Note that the number of computed columns might be larger than the size of the
   * nominal output column list as returned by outputColumns, as it might inlcude
   * internal columns that are needed for execution of upstream table expressions,
   * but should not be returned to the user
   */
  virtual size_t getNumComputedColumns() const = 0; //getNumComputerColumns

  /**
   * Returns the output column index for a named column or -1 if no such column
   * exists.
   *
   * If allow_add is set to true, the TableExpresionNode is allowed to add
   * the named column to the output list if the name refers to a valid column
   * and the column is not included in the output list yet.
   *
   * If allow_add is set to false, the TableExpression must return -1 even
   * if the column name refers to a valid column that is not included in the
   * output list yet
   */
  virtual size_t getColumnIndex( // getOutputColumnIndex
      const String& column_name,
      bool allow_add = false) = 0;


};

} // namespace csql
