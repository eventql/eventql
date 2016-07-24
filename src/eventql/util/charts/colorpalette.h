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
#ifndef _libstx_UI_COLORPALETTE_H
#define _libstx_UI_COLORPALETTE_H
#include <stdlib.h>

namespace util {
namespace chart {

class ColorPalette {
public:

  ColorPalette(
      const std::vector<std::string>& colors = std::vector<std::string>{
          "color1",
          "color2",
          "color3",
          "color4",
          "color5",
          "color6"}) :
          colors_(colors),
          color_index_(0) {}

  void setNextColor(Series* series) {
    series->setDefaultProperty(
        Series::P_COLOR,
        colors_[color_index_++ % colors_.size()]);
  }

protected:
  std::vector<std::string> colors_;
  size_t color_index_;
};

}
}
#endif
