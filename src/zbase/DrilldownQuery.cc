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
#include <csql/qtree/QueryTreeUtil.h>
#include <csql/runtime/runtime.h>

using namespace stx;

namespace zbase {

DrilldownQuery::DrilldownQuery(
    RefPtr<csql::ExecutionStrategy> execution_strategy,
    csql::Runtime* runtime) :
    execution_strategy_(execution_strategy),
    runtime_(runtime),
    txn_(runtime_->newContext()) {}

void DrilldownQuery::addMetric(MetricDefinition metric) {
  if (metric.name.isEmpty()) {
    metric.name = Some(StringUtil::toString(metrics_.size()));
  }

  if (!metric.name.isEmpty()) {
    metric_name_map_.emplace(metric.name.get(), metrics_.size());
  }

  metrics_.emplace_back(metric);
}

void DrilldownQuery::addDimension(DimensionDefinition dimension) {
  if (dimension.name.isEmpty()) {
    dimension.name = Some(StringUtil::toString(dimensions_.size()));
  }

  dimensions_.emplace_back(dimension);
}

void DrilldownQuery::setFilter(String filter) {
  filter_ = Some(filter);
}

RefPtr<DrilldownTree> DrilldownQuery::execute() {
  size_t ndims = dimensions_.size();
  auto dtree = mkRef(
      new DrilldownTree(
          ndims,
          metrics_.size(),
          1024 * 1024));

  auto result_handler = mkRef(new csql::CallbackResultHandler());
  result_handler->onRow([this, ndims, dtree] (
      size_t stmt_idx,
      int argc,
      const csql::SValue* argv) {
    if (argc != ndims + 1) {
      RAISE(kRuntimeError, "invalid result row");
    }

    auto node = dtree->lookupOrInsert(argv + 1);
    node->slots[stmt_idx] = *argv;
  });

  auto query_plan = buildQueryPlan();
  runtime_->executeQuery(txn_.get(), query_plan, result_handler.get());

  calculateDerivedMetrics(dtree);

  return dtree;
}

RefPtr<csql::QueryPlan> DrilldownQuery::buildQueryPlan() {
  Vector<RefPtr<csql::QueryTreeNode>> statements;

  for (const auto& metric : metrics_) {
    if (!metric.source_table.isEmpty()) {
      statements.emplace_back(buildQueryTree(metric));
    }
  }

  return runtime_->buildQueryPlan(txn_.get(), statements, execution_strategy_);
}

RefPtr<csql::QueryTreeNode> DrilldownQuery::buildQueryTree(
    const MetricDefinition& metric) {
  auto qtree_builder = runtime_->queryPlanBuilder();
  Vector<RefPtr<csql::SelectListNode>> outer_select_list;
  auto inner_sl = mkScoped(new csql::ASTNode(csql::ASTNode::T_SELECT_LIST));
  auto aggr_strategy = csql::AggregationStrategy::NO_AGGREGATION;

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

    qtree_builder->buildInternalSelectList(
        stmts[0],
        inner_sl.get());

    outer_select_list.emplace_back(
        new csql::SelectListNode(
            qtree_builder->buildValueExpression(stmts[0])));
  }

  // build dimension expressions
  Vector<RefPtr<csql::ValueExpressionNode>> group_exprs;
  for (const auto& dimension : dimensions_) {
    String dimexpr_str;
    if (!dimension.expression.isEmpty()) {
      dimexpr_str = dimension.expression.get();
    }

    if (dimexpr_str.empty()) {
      RAISEF(
          kRuntimeError,
          "missing expression for metric=$0 dimension=$1",
          metric.name.get(),
          dimension.name.get());
    }

    csql::Parser parser;
    parser.parseValueExpression(dimexpr_str.data(), dimexpr_str.size());

    auto stmts = parser.getStatements();
    if (stmts.size() != 1) {
      RAISE(kIllegalArgumentError);
    }

    qtree_builder->buildInternalSelectList(
        stmts[0],
        inner_sl.get());

    auto dimexpr = mkRef(qtree_builder->buildValueExpression(stmts[0]));
    outer_select_list.emplace_back(new csql::SelectListNode(dimexpr));
    group_exprs.emplace_back(dimexpr);
  }

  // build filter
  Option<RefPtr<csql::ValueExpressionNode>> where_expr;
  {
    String filter_str;
    if (!filter_.isEmpty()) {
      filter_str = "( " + filter_.get() + " )";
    }

    if (!metric.filter.isEmpty()) {
      if (!filter_str.empty()) {
        filter_str += " AND ";
      }

      filter_str += "( " + metric.filter.get() + " )";
    }


    if (!filter_str.empty()) {
      csql::Parser parser;
      parser.parseValueExpression(
          filter_str.data(),
          filter_str.size());

      auto stmts = parser.getStatements();
      if (stmts.size() != 1) {
        RAISE(kIllegalArgumentError);
      }

      where_expr = Some(mkRef(qtree_builder->buildValueExpression(stmts[0])));
    }
  }

  // build qtree node
  Vector<RefPtr<csql::SelectListNode>> inner_select_list;
  for (const auto& e : inner_sl->getChildren()) {
    inner_select_list.emplace_back(qtree_builder->buildSelectList(e));
  }

  if (qtree_builder->hasAggregationWithinRecord(inner_sl.get())) {
    aggr_strategy = csql::AggregationStrategy::AGGREGATE_WITHIN_RECORD_DEEP;
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

void DrilldownQuery::calculateDerivedMetrics(RefPtr<DrilldownTree> dtree) {
  auto qtree_builder = runtime_->queryPlanBuilder();
  auto query_builder = runtime_->queryBuilder();

  Vector<Pair<size_t, csql::ValueExpression>> exprs;
  for (size_t i = 0; i < metrics_.size(); ++i) {
    const auto& metric = metrics_[i];
    if (!metric.source_table.isEmpty()) {
      continue;
    }

    csql::Parser parser;
    parser.parseValueExpression(
        metric.expression.data(),
        metric.expression.size());

    auto stmts = parser.getStatements();
    if (stmts.size() != 1) {
      RAISE(kIllegalArgumentError);
    }

    auto expr = mkRef(qtree_builder->buildValueExpression(stmts[0]));
    auto resolver = [this] (const String& column_name) -> size_t {
      const auto& idx = metric_name_map_.find(column_name);

      if (idx == metric_name_map_.end()) {
        RAISEF(kNotFoundError, "metric not found: $0", column_name);
      }

      return idx->second;
    };

    csql::QueryTreeUtil::resolveColumns(expr, resolver);
    exprs.emplace_back(i, query_builder->buildValueExpression(txn_.get(), expr));
  }

  dtree->walkLeafs([this, &exprs] (DrilldownTreeLeafNode* node) {
    for (const auto& e : exprs) {
      node->slots[e.first] = runtime_->evaluateScalarExpression(
          txn_.get(),
          e.second,
          node->slots.size(),
          node->slots.data());
    }
  });
}

} // namespace zbase

