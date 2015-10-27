/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/option.h>
#include <csql/qtree/TableExpressionNode.h>
#include <csql/qtree/ValueExpressionNode.h>
#include <csql/qtree/SelectListNode.h>

using namespace stx;

namespace csql {

/**
 * If scanning a table this flag controls the aggregation behaviour and how many
 * rows are emitted:
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

class SequentialScanNode : public TableExpressionNode {
public:

  SequentialScanNode(
      const String& table_name,
      Vector<RefPtr<SelectListNode>> select_list,
      Option<RefPtr<ValueExpressionNode>> where_expr);

  SequentialScanNode(
      const String& table_name,
      Vector<RefPtr<SelectListNode>> select_list,
      Option<RefPtr<ValueExpressionNode>> where_expr,
      AggregationStrategy aggr_strategy);

  SequentialScanNode(const SequentialScanNode& other);

  const String& tableName() const;
  void setTableName(const String& table_name);

  Vector<RefPtr<SelectListNode>> selectList() const;
  Set<String> selectedColumns() const;
  Vector<String> columnNames() const;

  Option<RefPtr<ValueExpressionNode>> whereExpression() const;

  AggregationStrategy aggregationStrategy() const;
  void setAggregationStrategy(AggregationStrategy strategy);

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toString() const override;

protected:
  String table_name_;
  Vector<RefPtr<SelectListNode>> select_list_;
  Option<RefPtr<ValueExpressionNode>> where_expr_;
  AggregationStrategy aggr_strategy_;
};

} // namespace csql
