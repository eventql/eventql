/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <eventql/sql/runtime/charts/barchartbuilder.h>
#include <eventql/sql/runtime/charts/drawstatement.h>
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
