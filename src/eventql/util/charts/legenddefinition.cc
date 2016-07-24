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
#include "eventql/util/charts/legenddefinition.h"

namespace util {
namespace chart {

LegendDefinition::LegendDefinition(
    kVerticalPosition vert_pos,
    kHorizontalPosition horiz_pos,
    kPlacement placement,
    const std::string& title) :
    vert_pos_(vert_pos),
    horiz_pos_(horiz_pos),
    placement_(placement),
    title_ (title) {}

const std::string LegendDefinition::title() const {
  return title_;
}

LegendDefinition::kVerticalPosition LegendDefinition::verticalPosition() 
    const {
  return vert_pos_;
}

LegendDefinition::kHorizontalPosition LegendDefinition::horizontalPosition() 
    const {
  return horiz_pos_;
}

LegendDefinition::kPlacement LegendDefinition::placement() const {
  return placement_;
}

void LegendDefinition::addEntry(
    const std::string& name,
    const std::string& color,
    const std::string& shape /* = "circle" */) {
  entries_.emplace_back(name, color, shape);
}

const std::vector<std::tuple<std::string, std::string, std::string>>
    LegendDefinition::entries() const {
  return entries_;
}

}
}
