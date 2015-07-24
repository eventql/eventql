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
#include <chartsql/qtree/DrawStatementNode.h>
#include <chartsql/parser/token.h>
#include <fnord/exception.h>
#include <fnord/autoref.h>
#include <fnord/charts/canvas.h>
#include <fnord/charts/drawable.h>

namespace csql {
class Runtime;

class DrawStatement : public Statement {
public:

  DrawStatement(
      RefPtr<DrawStatementNode> node,
      Vector<ScopedPtr<TableExpression>> sources,
      Runtime* runtime);

  void execute(
      ExecutionContext* context,
      fnord::chart::Canvas* canvas);

protected:

  template <typename ChartBuilderType>
  fnord::chart::Drawable* executeWithChart(
      ExecutionContext* context,
      fnord::chart::Canvas* canvas) {
    ChartBuilderType chart_builder(canvas, node_);

    for (auto& source : sources_) {
      chart_builder.executeStatement(source.get(), context);
    }

    return chart_builder.getChart();
  }

  void applyAxisDefinitions(fnord::chart::Drawable* chart) const;
  void applyAxisLabels(ASTNode* ast, fnord::chart::AxisDefinition* axis) const;
  void applyDomainDefinitions(fnord::chart::Drawable* chart) const;
  void applyGrid(fnord::chart::Drawable* chart) const;
  void applyLegend(fnord::chart::Drawable* chart) const;
  void applyTitle(fnord::chart::Drawable* chart) const;

  RefPtr<DrawStatementNode> node_;
  Vector<ScopedPtr<TableExpression>> sources_;
  Runtime* runtime_;
};

}
