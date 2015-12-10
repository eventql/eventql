/**
 * Copyright (c) 2015 - zScale Technology GmbH <legal@zscale.io>
 *   All Rights Reserved.
 *
 * Authors:
 *   Paul Asmuth <paul@zscale.io>
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <zbase/DrilldownQuery.h>
#include <csql/parser/parser.h>
#include <csql/parser/astnode.h>
#include <csql/runtime/runtime.h>

using namespace stx;

namespace zbase {

DrilldownQuery::DrilldownQuery(
      RefPtr<csql::TableProvider> table_provider,
      csql::Runtime* runtime) :
      table_provider_(table_provider),
      runtime_(runtime) {}

void DrilldownQuery::addMetric(MetricDefinition metric) {
  metrics_.emplace_back(metric);
}

void DrilldownQuery::addDimension(DimensionDefinition dimension) {
  dimensions_.emplace_back(dimension);
}

void DrilldownQuery::setFilter(String filter) {
  filter_ = Some(filter);
}

void DrilldownQuery::execute() {

}

RefPtr<csql::QueryTreeNode> DrilldownQuery::buildQueryTree(
    const MetricDefinition& metric) {
  Vector<RefPtr<csql::SelectListNode>> outer_select_list;
  Vector<RefPtr<csql::SelectListNode>> inner_select_list;
  Vector<RefPtr<csql::ValueExpressionNode>> group_exprs;
  Option<RefPtr<csql::ValueExpressionNode>> where_expr;
  auto aggr_strategy = csql::AggregationStrategy::NO_AGGREGATION;

  auto qtree_builder = runtime_->queryPlanBuilder();

  // build result expression
  {
    csql::Parser parser;
    parser.parseValueExpression(
        metric.expression.data(),
        metric.expression.size());

    auto stmts = parser.getStatements();
    if (stmts.size() != 1) {
      RAISE(kIllegalArgumentError);
    }

    if (qtree_builder->hasAggregationWithinRecord(stmts[0])) {
      aggr_strategy = csql::AggregationStrategy::AGGREGATE_WITHIN_RECORD_DEEP;
    }

    outer_select_list.emplace_back(
        new csql::SelectListNode(
            qtree_builder->buildValueExpression(stmts[0])));

    auto inner_sl = mkScoped(new csql::ASTNode(csql::ASTNode::T_SELECT_LIST));
    qtree_builder->buildInternalSelectList(
        stmts[0],
        inner_sl.get());

    for (const auto& e : inner_sl->getChildren()) {
      inner_select_list.emplace_back(qtree_builder->buildSelectList(e));
    }
  }

  return new csql::GroupByNode(
      outer_select_list,
      group_exprs,
      new csql::SequentialScanNode(
            metric.source_table.get(),
            inner_select_list,
            where_expr,
            aggr_strategy));
}

} // namespace zbase

