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
#ifndef _libstx_RENDERTARGET_H
#define _libstx_RENDERTARGET_H
#include <stdlib.h>
#include <vector>
#include <string>

namespace util {
namespace chart {

class RenderTarget {
public:
  virtual ~RenderTarget() {}

  virtual void beginChart(
      int width,
      int height,
      const std::string& class_name) = 0;

  virtual void finishChart() = 0;

  virtual void beginGroup(const std::string& class_name) = 0;
  virtual void finishGroup() = 0;

  virtual void drawLine(
      double x1,
      double y1,
      double x2,
      double y2,
      const std::string& class_name) = 0;

  virtual void drawText(
      const std::string& text,
      double x,
      double y,
      const std::string& halign,
      const std::string& valign,
      const std::string& class_name,
      double rotate = 0.0f) = 0;

  virtual void drawPoint(
      double x,
      double y,
      const std::string& point_type,
      double point_size,
      const std::string& color,
      const std::string& class_name = "",
      const std::string& label = "",
      const std::string& series = "") = 0;

  virtual void drawRect(
      double x,
      double y,
      double width,
      double height,
      const std::string& color,
      const std::string& class_name = "",
      const std::string& label = "",
      const std::string& series = "") = 0;

  virtual void drawPath(
      const std::vector<std::pair<double, double>>& points,
      const std::string& line_style,
      double line_width,
      bool smooth,
      const std::string& color,
      const std::string& class_name = "") = 0;

};


}
}
#endif
