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
#ifndef _libstx_UI_VIEWPORT_H
#define _libstx_UI_VIEWPORT_H

namespace util {
namespace chart {

class Viewport {
public:

  Viewport(
      int width,
      int height) :
      Viewport(width, height, std::tuple<int, int, int, int>(0,0,0,0)) {}

  Viewport(
      int width,
      int height,
      const std::tuple<int, int, int, int>& padding) :
      width_(width),
      height_(height),
      padding_(padding) {}

  int innerWidth() const {
    return width_ - paddingRight() - paddingLeft();
  }

  int innerHeight() const {
    return height_ - paddingTop() - paddingBottom();
  }

  int paddingTop() const {
    return std::get<0>(padding_);
  }

  int paddingRight() const {
    return std::get<1>(padding_);
  }

  int paddingBottom() const {
    return std::get<2>(padding_);
  }

  int paddingLeft() const {
    return std::get<3>(padding_);
  }

  const std::tuple<int, int, int, int>& padding() {
    return padding_;
  }

  void setPadding(const std::tuple<int, int, int, int>& padding) {
    padding_ = padding;
  }

  void setPaddingTop(int val) {
    std::get<0>(padding_) = val;
  }

  void setPaddingRight(int val) {
    std::get<1>(padding_) = val;
  }

  void setPaddingBottom(int val) {
    std::get<2>(padding_) = val;
  }

  void setPaddingLeft(int val) {
    std::get<3>(padding_) = val;
  }

protected:
  int width_;
  int height_;
  std::tuple<int, int, int, int> padding_;
};

}
}
#endif
