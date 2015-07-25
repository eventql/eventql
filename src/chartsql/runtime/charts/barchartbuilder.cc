/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/charts/barchartbuilder.h>
#include <chartsql/runtime/charts/drawstatement.h>
#include <cplot/barchart.h>

namespace csql {

BarChartBuilder::BarChartBuilder(
    stx::chart::Canvas* canvas,
    RefPtr<DrawStatementNode> draw_stmt) :
    ChartBuilder(canvas, draw_stmt) {}

stx::chart::Drawable* BarChartBuilder::getChart() const {
  auto chart = dynamic_cast<stx::chart::BarChart*>(findChartType());
  setOrientation(chart);
  setStacked(chart);
  setLabels(chart);
  return chart;
}

stx::chart::Drawable* BarChartBuilder::findChartType() const {
  preconditionCheck();

  if (auto c = tryType2D<stx::chart::BarChart2D<
        SValue::StringType,
        SValue::FloatType>>())
    return c;

  if (auto c = tryType3D<stx::chart::BarChart3D<
        SValue::StringType,
        SValue::FloatType,
        SValue::FloatType>>())
    return c;

  invalidType();
  return nullptr;
}

void BarChartBuilder::setOrientation(stx::chart::BarChart* chart) const {
  auto prop = draw_stmt_->getProperty(Token::T_ORIENTATION);

  if (prop == nullptr) {
    return;
  }

  switch (prop->getToken()->getType()) {
    case Token::T_VERTICAL:
      chart->setOrientation(stx::chart::BarChart::O_VERTICAL);
      break;

    case Token::T_HORIZONTAL:
      chart->setOrientation(stx::chart::BarChart::O_HORIZONTAL);
      break;

    default:
      break;
  }
}

void BarChartBuilder::setStacked(stx::chart::BarChart* chart) const {
  auto prop = draw_stmt_->getProperty(Token::T_STACKED);
  chart->setStacked(prop != nullptr);
}

std::string BarChartBuilder::chartName() const {
  return "BarChart";
}

void BarChartBuilder::setLabels(stx::chart::BarChart* chart) const {
  auto prop = draw_stmt_->getProperty(Token::T_LABELS);
  chart->setLabels(prop != nullptr);
}

}
