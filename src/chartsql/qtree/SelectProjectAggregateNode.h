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
#include <fnord/stdtypes.h>
#include <chartsql/qtree/QueryTreeNode.h>
#include <chartsql/qtree/ScalarExpressionNode.h>

using namespace fnord;

namespace csql {

class SelectProjectAggregateNode : public StatementNode {
public:

  SelectProjectAggregateNode(
      Vector<RefPtr<ScalarExpressionNode>> select_list,
      RefPtr<ScalarExpressionNode> where_expr);

  Vector<RefPtr<ScalarExpressionNode>> selectList() const;

  /**
   * If scanning a table that has nested records as columns/rows, this flag
   * controls the behaviour:
   *
   *   - if true: return one row for each leaf record that was referenced
   *   - if false: return one row for each source row (allowing WITHIN RECORD
   *     aggregations)
   */
  bool expandNestedRecords() const;

  void setExpandNestedRecords(bool expand);

protected:
  Vector<RefPtr<ScalarExpressionNode>> select_list_;
  RefPtr<ScalarExpressionNode> where_expr_;
  bool expand_nested_records_;
};

} // namespace csql
