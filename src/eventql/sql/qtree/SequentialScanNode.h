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
#include <eventql/sql/svalue.h>
#include <eventql/sql/qtree/TableExpressionNode.h>
#include <eventql/sql/qtree/ValueExpressionNode.h>
#include <eventql/sql/qtree/SelectListNode.h>
#include <eventql/sql/TableInfo.h>
#include <eventql/sql/qtree/qtree_coder.h>

#include "eventql/eventql.h"

namespace csql {
class TableProvider;

/**
 * This flag controls the table scan aggregation behaviour and how many rows are
 * emitted:
 *
 *   NO_AGGREGATION:
 *       - emit one row per input row without any aggregation
 *       - if table is flat: return rows 1:1 from the input table
 *       - if table has nested records: return one row for each outermost leaf
 *         record that was referenced
 *
 *   AGGREGATE_WITHIN_RECORD:
 *       - if table is flat: run each aggregation function on every input row
 *         individually (this is pointless) and return one row per input row
 *       - if table has nested records: run each aggregation function on all
 *         rows from the same record and return one row for each input record
 *
 *   AGGREGATE_ALL:
 *       - run each aggregation function on all scanned rows. returns exactly
 *        one output row
 *
 */
enum class AggregationStrategy : uint8_t {
  NO_AGGREGATION = 0,
  AGGREGATE_WITHIN_RECORD_FLAT = 1,
  AGGREGATE_WITHIN_RECORD_DEEP = 2,
  AGGREGATE_ALL = 3
};

enum class ScanConstraintType : uint8_t {
  EQUAL_TO,
  NOT_EQUAL_TO,
  LESS_THAN,
  LESS_THAN_OR_EQUAL_TO,
  GREATER_THAN,
  GREATER_THAN_OR_EQUAL_TO
};

/**
 * Describes a "constraint" on a sequential scan, e.g. a hard limit that was
 * derived from the WHERE clause and can be used by the executing storage engine
 * to limit the search over rows.
 *
 * There are currently 6 types of constraints implemented. All follow the same
 * schema: "column <OP> value" where `column` is a reference to a real column in
 * the underlying table, `value` is any SValue and `OP` is one of "EQUAL_TO",
 * "NOT_EQUAL_TO", "LESS_THAN", "LESS_THAN_OR_EQUAL_TO", "GREATER_THAN",
 * "GREATER_THAN_OR_EQUAL_TO"
 */
struct ScanConstraint {
  String column_name;
  ScanConstraintType type;
  SValue value;

  bool operator==(const ScanConstraint& other) const;
  bool operator!=(const ScanConstraint& other) const;
};

class SequentialScanNode : public TableExpressionNode {
public:

  SequentialScanNode(
      const TableInfo& table_info,
      RefPtr<TableProvider> table_provider,
      Vector<RefPtr<SelectListNode>> select_list,
      Option<RefPtr<ValueExpressionNode>> where_expr);

  SequentialScanNode(
      const TableInfo& table_info,
      RefPtr<TableProvider> table_provider,
      Vector<RefPtr<SelectListNode>> select_list,
      Option<RefPtr<ValueExpressionNode>> where_expr,
      AggregationStrategy aggr_strategy);

  SequentialScanNode(const SequentialScanNode& other);

  const String& tableName() const;
  const String& tableAlias() const;
  void setTableName(const String& table_name);
  void setTableAlias(const String& table_alias);

  Vector<RefPtr<SelectListNode>> selectList() const;
  Set<String> selectedColumns() const;

  Vector<String> outputColumns() const override;

  Vector<QualifiedColumn> allColumns() const override;

  void normalizeColumnNames();
  String normalizeColumnName(const String& column_name) const;

  size_t getColumnIndex(
      const String& column_name,
      bool allow_add = false) override;

  size_t getNumComputedColumns() const override;

  Option<RefPtr<ValueExpressionNode>> whereExpression() const;
  void setWhereExpression(RefPtr<ValueExpressionNode> e);

  /**
   * Returns all constraints (see ScanConstraint) that were derived from the
   * WHERE expression
   */
  const Vector<ScanConstraint>& constraints() const;

  AggregationStrategy aggregationStrategy() const;
  void setAggregationStrategy(AggregationStrategy strategy);

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toString() const override;

  static void encode(
      QueryTreeCoder* coder,
      const SequentialScanNode& node,
      OutputStream* os);

  static RefPtr<QueryTreeNode> decode(
      QueryTreeCoder* coder,
      InputStream* os);

protected:

  void findSelectedColumnNames(
    RefPtr<ValueExpressionNode> expr,
    Set<String>* columns) const;

  String table_name_;
  String table_alias_;
  Vector<String> table_columns_;
  RefPtr<TableProvider> table_provider_;
  Vector<RefPtr<SelectListNode>> select_list_;
  Vector<String> output_columns_;
  Option<RefPtr<ValueExpressionNode>> where_expr_;
  AggregationStrategy aggr_strategy_;
  Vector<ScanConstraint> constraints_;
};

} // namespace csql
