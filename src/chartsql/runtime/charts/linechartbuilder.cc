/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/charts/linechartbuilder.h>
#include <chartsql/runtime/charts/drawstatement.h>

namespace csql {

LineChartBuilder::LineChartBuilder(
    fnord::chart::Canvas* canvas,
    DrawStatement const* draw_stmt) :
    ChartBuilder(canvas, draw_stmt) {}

fnord::chart::Drawable* LineChartBuilder::getChart() const {
  auto chart = dynamic_cast<fnord::chart::LineChart*>(findChartType());
  setLabels(chart);
  return chart;
}

fnord::chart::Drawable* LineChartBuilder::findChartType() const {
  preconditionCheck();

  if (auto c = tryType2D<fnord::chart::LineChart2D<
        SValue::TimeType,
        SValue::TimeType>>())
    return c;

  if (auto c = tryType2D<fnord::chart::LineChart2D<
        SValue::TimeType,
        SValue::FloatType>>())
    return c;

  if (auto c = tryType2D<fnord::chart::LineChart2D<
        SValue::TimeType,
        SValue::StringType>>())
    return c;

  if (auto c = tryType2D<fnord::chart::LineChart2D<
        SValue::FloatType,
        SValue::StringType>>())
    return c;

  if (auto c = tryType2D<fnord::chart::LineChart2D<
        SValue::FloatType,
        SValue::FloatType>>())
    return c;

  if (auto c = tryType2D<fnord::chart::LineChart2D<
        SValue::FloatType,
        SValue::TimeType>>())
    return c;

  if (auto c = tryType2D<fnord::chart::LineChart2D<
        SValue::FloatType,
        SValue::StringType>>())
    return c;

  if (auto c = tryType2D<fnord::chart::LineChart2D<
        SValue::StringType,
        SValue::TimeType>>())
    return c;

  if (auto c = tryType2D<fnord::chart::LineChart2D<
        SValue::StringType,
        SValue::FloatType>>())
    return c;

  if (auto c = tryType2D<fnord::chart::LineChart2D<
        SValue::StringType,
        SValue::StringType>>())
    return c;

  invalidType();
  return nullptr;
}

std::string LineChartBuilder::chartName() const {
  return "LineChart";
}

void LineChartBuilder::setLabels(fnord::chart::LineChart* chart) const {
  auto prop = draw_stmt_->getProperty(Token::T_LABELS);
  chart->setLabels(prop != nullptr);
}

}
