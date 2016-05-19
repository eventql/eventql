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
#ifndef _libstx_SERIESJOIN_H
#define _libstx_SERIESJOIN_H
#include <stdlib.h>
#include <string>
#include <vector>

namespace util {
namespace chart {

template <typename TX, typename TY, typename TZ>
class SeriesJoin3D {
public:
  struct JoinedPoint {
    JoinedPoint(const Series::Coord<TX>& x_) : x(x_) {}
    Series::Coord<TX> x;
    std::string label;
    std::vector<std::pair<Series::Coord<TY>, Series::Coord<TZ>>> ys;
  };

  SeriesJoin3D() : num_series_(0) {}

  // FIXPAUL this should not be O(n^2)
  void addSeries(Series3D<TX, TY, TZ>* series) {
    for (const auto& point : series->getData()) {
      addPoint(point, series->labelFor(&point));
    }

    for (auto& joined : data_) {
      for (int i = joined.ys.size(); i <= num_series_; ++i) {
        joined.ys.emplace_back(
            Series::Coord<TY>(nullptr),
            Series::Coord<TZ>(nullptr));
      }
    }

    num_series_++;
  }

  void addPoint(
      const typename Series3D<TX, TY, TZ>::Point& point,
      const std::string& label,
      bool force = false) {
    JoinedPoint* joined = nullptr;
    for (auto& candidate : data_) {
      if (candidate.x == point.x_coord()) {
        joined = &candidate;
      }
    }

    if (joined == nullptr) {
      data_.emplace_back(point.x_coord());
      joined = &data_.back();
    }

    for (int i = joined->ys.size(); i < num_series_; ++i) {
      joined->ys.emplace_back(
          Series::Coord<TY>(nullptr),
          Series::Coord<TZ>(nullptr));
    }

    if (joined->ys.size() < num_series_ + 1 || force) {
      joined->ys.emplace_back(point.y_coord(), point.z_coord());
    }

    if (joined->label.size() == 0) {
      joined->label = label;
    } else if (joined->label != label) {
      joined->label.append(", ");
      joined->label.append(label);
    }
  }

  size_t size() const {
    return data_.size();
  }

  const std::vector<JoinedPoint>& getData() const {
    return data_;
  }

  size_t seriesCount() const {
    return num_series_;
  }

  void setSeriesCount(int num_series) {
    num_series_ = num_series;
  }

protected:
  std::vector<JoinedPoint> data_;
  int num_series_;
};


}
}
#endif
