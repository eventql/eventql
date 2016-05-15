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
#ifndef _libstx_DRAWABLE_H
#define _libstx_DRAWABLE_H
#include <tuple>
#include <functional>
#include "cplot/axisdefinition.h"
#include "cplot/griddefinition.h"
#include "cplot/legenddefinition.h"
#include "cplot/series.h"
#include "cplot/viewport.h"

namespace util {
namespace chart {
class RenderTarget;
class Canvas;

// FIXPAUL: rename to chart
class Drawable {
  friend class Canvas;
public:
  Drawable(Canvas* canvas);
  virtual ~Drawable();

  /**
   * Set the title for this chart
   */
  void setTitle(const std::string& title);

  /**
   * Set the subtitle for this chart
   */
  void setSubtitle(const std::string& subtitle);

  /**
   * Add an axis to the chart.
   *
   * The returned pointer is owned by the canvas object and must not be freed
   * by the caller!
   *
   * @param position the position/placement of the axis
   */
  virtual AxisDefinition* addAxis(AxisDefinition::kPosition position) = 0;

  /**
   * Add a grid to the chart.
   *
   * The returned pointer is owned by the canvas object and must not be freed
   * by the caller!
   *
   * @param vertical render vertical grid lines?
   * @param horizontal render horizonyal grid lines?
   */
  virtual GridDefinition* addGrid(GridDefinition::kPlacement placement) = 0;

  /**
   * Add a legend to the chart.
   *
   * The returned pointer is owned by the canvas object and must not be freed
   * by the caller!
   *
   * FIXPAUL params
   */
  LegendDefinition* addLegend(
      LegendDefinition::kVerticalPosition vert_pos,
      LegendDefinition::kHorizontalPosition horiz_pos,
      LegendDefinition::kPlacement placement,
      const std::string& title);

  /**
   * Get the {x,y,z} domain of this chart. May raise an exception if the chart
   * does not implement the requested domain.
   *
   * The returned pointer is owned by the canvas object and must not be freed
   * by the caller!
   *
   * @param dimension the dimension
   */
  virtual AnyDomain* getDomain(AnyDomain::kDimension dimension) = 0;

protected:

  void addSeries(Series* series);
  virtual void render(RenderTarget* target, Viewport* viewport) const = 0;

  Canvas* canvas_;
private:
  void updateLegend();
  std::vector<Series*> all_series_;
};

}
}
#endif
