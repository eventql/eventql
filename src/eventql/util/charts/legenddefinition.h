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
#ifndef _libstx_UI_LEGENDDEFINITION_H
#define _libstx_UI_LEGENDDEFINITION_H
#include <tuple>
#include <string>
#include <vector>

namespace util {
namespace chart {

class LegendDefinition {
public:

  enum kVerticalPosition {
    LEGEND_TOP = 0,
    LEGEND_BOTTOM = 1
  };

  enum kHorizontalPosition {
    LEGEND_LEFT = 0,
    LEGEND_RIGHT = 1
  };

  enum kPlacement {
    LEGEND_INSIDE = 0,
    LEGEND_OUTSIDE = 1
  };

  /**
   * Create a new legend definition
   */
  LegendDefinition(
      kVerticalPosition vert_pos,
      kHorizontalPosition horiz_pos,
      kPlacement placement,
      const std::string& title);

  const std::string title() const;
  kVerticalPosition verticalPosition() const;
  kHorizontalPosition horizontalPosition() const;
  kPlacement placement() const;

  void addEntry(
      const std::string& name,
      const std::string& color,
      const std::string& shape = "circle");

  const std::vector<std::tuple<std::string, std::string, std::string>>
      entries() const;

protected:
  kVerticalPosition vert_pos_;
  kHorizontalPosition horiz_pos_;
  kPlacement placement_;
  const std::string title_;
  std::vector<std::tuple<std::string, std::string, std::string>> entries_;
};

}
}
#endif
