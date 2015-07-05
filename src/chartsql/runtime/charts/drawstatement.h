/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/stdtypes.h>
#include <chartsql/runtime/TableExpression.h>
#include <chartsql/qtree/drawnode.h>
#include <chartsql/parser/Token.h>
#include <fnord/exception.h>
#include <fnord/autoref.h>
#include <fnord/charts/canvas.h>
#include <fnord/charts/drawable.h>

namespace csql {
class Runtime;

class DrawStatement : public TableExpression {
public:

  DrawStatement(
      RefPtr<DrawNode> node,
      Vector<ScopedPtr<TableExpression>> sources,
      Runtime* runtime);

  void execute(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn) override;

  Vector<String> columnNames() const override;

  size_t numColumns() const override;

protected:

  template <typename ChartBuilderType>
  fnord::chart::Drawable* executeWithChart(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn) {
    ChartBuilderType chart_builder(&canvas_, this);

    //for (size_t i = 0; i < sources_.size(); ++i) {
    //  const auto& stmt = select_stmts_[i];
    //  chart_builder.executeStatement(stmt, result_lists_[i]);
    //}

    return chart_builder.getChart();
  }

  //void applyAxisDefinitions(fnord::chart::Drawable* chart) const;
  //void applyAxisLabels(ASTNode* ast, fnord::chart::AxisDefinition* axis) const;
  //void applyDomainDefinitions(fnord::chart::Drawable* chart) const;
  //void applyGrid(fnord::chart::Drawable* chart) const;
  //void applyLegend(fnord::chart::Drawable* chart) const;
  //void applyTitle(fnord::chart::Drawable* chart) const;

  //std::vector<QueryPlanNode*> select_stmts_;
  //std::vector<ResultList*> result_lists_;
  RefPtr<DrawNode> node_;
  Vector<ScopedPtr<TableExpression>> sources_;
  Runtime* runtime_;
  fnord::chart::Canvas canvas_;
};

struct ChartStatement {
  Vector<RefPtr<DrawStatement>> draw_statements;
  void render(fnord::chart::RenderTarget* target) const;
};

}
