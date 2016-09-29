/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <string>
#include "eventql/util/charts/canvas.h"
#include "eventql/util/charts/drawable.h"

namespace util {
namespace chart {

Drawable::Drawable(Canvas* canvas) : canvas_(canvas) {}

Drawable::~Drawable() {
  for (auto series : all_series_) {
    delete series;
  }
}

void Drawable::setTitle(const std::string& title) {
  canvas_->setTitle(title);
}

void Drawable::setSubtitle(const std::string& subtitle) {
  canvas_->setSubtitle(subtitle);
}

LegendDefinition* Drawable::addLegend(
    LegendDefinition::kVerticalPosition vert_pos,
    LegendDefinition::kHorizontalPosition horiz_pos,
    LegendDefinition::kPlacement placement,
    const std::string& title) {
  auto legend = canvas_->addLegend(vert_pos, horiz_pos, placement, title);
  updateLegend();
  return legend;
}

void Drawable::addSeries(Series* series) {
  all_series_.push_back(series);
  updateLegend();
}

void Drawable::updateLegend() {
  auto legend = canvas_->legend();

  if (legend == nullptr) {
    return;
  }

  for (const auto& series : all_series_) {
    legend->addEntry(
        series->name(),
        series->getProperty(Series::P_COLOR),
        "circle");
  }
}

}
}
