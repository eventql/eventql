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
#pragma once
#include <stdlib.h>
#include <assert.h>
#include <unordered_map>
#include <eventql/util/charts/canvas.h>
#include <eventql/util/charts/barchart.h>
#include <eventql/util/charts/series.h>
#include <eventql/util/exception.h>
#include <eventql/sql/runtime/compiler.h>
#include <eventql/sql/runtime/vm.h>

namespace csql {

class AnySeriesAdapter {
public:

  AnySeriesAdapter(
      int name_ind,
      int x_ind,
      int y_ind,
      int z_ind);

  virtual ~AnySeriesAdapter() = default;

  virtual bool nextRow(SValue* row, int row_len) = 0;

  int name_ind_;
  int x_ind_;
  int y_ind_;
  int z_ind_;
  std::vector<std::pair<util::chart::Series::kProperty, int>> prop_indexes_;

protected:

  void applyProperties(
      SValue* row,
      int row_len,
      util::chart::Series* series,
      util::chart::Series::AnyPoint* point);

};

template <typename TX, typename TY>
class SeriesAdapter2D : public AnySeriesAdapter {
public:

  SeriesAdapter2D(
      int name_ind,
      int x_ind,
      int y_ind) :
      AnySeriesAdapter(name_ind, x_ind, y_ind, -1) {}

  bool nextRow(SValue* row, int row_len) override {
    std::string name = "unnamed";

    if (name_ind_ >= 0) {
      name = row[name_ind_].template getValue<std::string>();
    }

    util::chart::Series2D<TX, TY>* series = nullptr;
    const auto& series_iter = series_map_.find(name);
    if (series_iter == series_map_.end()) {
      series = new util::chart::Series2D<TX, TY>(name);
      series_map_.emplace(name, series);
      series_list_.emplace_back(series);
    } else {
      series = series_iter->second;
    }

    if (row[y_ind_].getType() == SType::NIL) {
      // FIXPAUL better handling of missing/NULL values!
    } else {
      series->addDatum(
          row[x_ind_].template getValue<TX>(),
          row[y_ind_].template getValue<TY>());
    }

    applyProperties(
        row,
        row_len,
        series,
        &series->getData().back());

    return true;
  }

  std::unordered_map<std::string, util::chart::Series2D<TX, TY>*> series_map_;
  std::vector<util::chart::Series2D<TX, TY>*> series_list_;
};

template <typename TX, typename TY, typename TZ>
class SeriesAdapter3D : public AnySeriesAdapter {
public:

  SeriesAdapter3D(
      int name_ind,
      int x_ind,
      int y_ind,
      int z_ind) :
      AnySeriesAdapter(name_ind, x_ind, y_ind, z_ind) {}

  bool nextRow(SValue* row, int row_len) override {
    std::string name = "unnamed";

    if (name_ind_ >= 0) {
      name = row[name_ind_].template getValue<std::string>();
    }

    util::chart::Series3D<TX, TY, TZ>* series = nullptr;
    const auto& series_iter = series_map_.find(name);
    if (series_iter == series_map_.end()) {
      series = new util::chart::Series3D<TX, TY, TZ>(name);
      series_map_.emplace(name, series);
      series_list_.emplace_back(series);
    } else {
      series = series_iter->second;
    }

    series->addDatum(
        row[x_ind_].template getValue<TX>(),
        row[y_ind_].template getValue<TY>(),
        row[z_ind_].template getValue<TZ>());

    applyProperties(
        row,
        row_len,
        series,
        &series->getData().back());

    return true;
  }

  std::unordered_map<
      std::string,
      util::chart::Series3D<TX, TY, TZ>*> series_map_;

  std::vector<util::chart::Series3D<TX, TY, TZ>*> series_list_;
};

}
